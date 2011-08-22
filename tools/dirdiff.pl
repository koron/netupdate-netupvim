#!/usr/bin/perl
# vim:set ts=8 sts=4 sw=4 tw=0:
#
# Last Change: 06-Oct-2001.
# Written By:  Muraoka Taro <koron@tka.att.ne.jp>

use strict;
use Digest::MD5;

sub md5_file
{
    my ($name) = @_;
    my $md5 = new Digest::MD5;

    open MD5_TARGET, $name;
    binmode MD5_TARGET;
    $md5->addfile(*MD5_TARGET);
    close MD5_TARGET;

    return $md5->hexdigest();
}

sub dirlist
{
    my ($path) = @_;
    my $ls = [];
    my $dir = $path eq '.' ? '' : "$path/";

    opendir LS, $path;
    my @ls = sort {$a cmp $b} readdir LS;
    closedir LS;

    foreach (@ls)
    {
	next if /^\.\.?$/;
	my $name = $dir . $_;
	if (-d $name)
	{
	    push @$ls, @{&dirlist($name)};
	}
	else
	{
	    my $obj = {
		name=>$name,
		size=>-s _,
		md5=>&md5_file($name),
	    };
	    push @$ls, $obj;
	    #printf "%7d %s %s\n", $obj->{size}, $obj->{md5}, $obj->{name};
	}
    }

    return $ls;
}

sub mkdir_p
{
    my ($path, $mode) = @_;
    $mode = 0777 if !defined $mode;
    my (@path) = split /[\/\\]/, $path;

    $path = '';
    for (@path)
    {
	$path .= $_ . '/';
	mkdir $path, $mode;
    }
}

sub copy
{
    my ($src, $dest) = @_;
    (my $destdir = $dest) =~ s/[\/\\][^\/\\]+$//;
    &mkdir_p($destdir, 0755);
    #link $src, $dest;
    system "cp -p $src $dest";
}

# 引数の受け取り
my ($srcdir, $destdir) = ('', '');
$srcdir  = $ARGV[0] if defined $ARGV[0];
$destdir = $ARGV[1] if defined $ARGV[1];
my $outdir = '';
$outdir  = $ARGV[2] if defined $ARGV[2];

# 引数の内容・整合性をチェック
$srcdir =~ s/[\/\\]$//;
$destdir =~ s/[\/\\]$//;
if ($srcdir eq '' or !-d $srcdir)
{
    printf STDERR "$0: Source directory is invalid.\n";
    exit -1;
}
if ($destdir eq '' or !-d $destdir)
{
    printf STDERR "$0: Destination directory is invalid.\n";
    exit -1;
}

# ディレクトリ情報収集
printf STDERR "Correcting source directory information...\n";
my $src  = &dirlist($srcdir);
printf STDERR "Correcting destination directory information...\n";
my $dest = &dirlist($destdir);
my %src = map {
    (my $name = $_->{'name'}) =~ s/^\Q$srcdir\E/$destdir/o;
    ($name, $_);
} @$src;

# 差分計算
my (@updated, @deleted);
for (@$dest)
{
    my ($name, $md5) = @{$_}{'name', 'md5'};
    next if exists $src{$name} and $src{$name}->{'md5'} eq $md5;

    push @updated, $_;
}

# 差分結果表示
if ($outdir eq '' or $outdir eq $destdir)
{
    for (@updated)
    {
	my ($name, $md5) = @{$_}{'name', 'md5'};
	printf "$md5 $name\n" if $outdir eq '';
    }
}
else
{
    print "Output to $outdir\n";
    for (@updated)
    {
	my $src = $_->{'name'};
	(my $dest = $src) =~ s/^\Q$destdir\E/$outdir/o;
	&copy($src, $dest);
	#print "src=$src, dest=$dest\n";
    }
}
