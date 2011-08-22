#!/usr/bin/perl
# vim:set ts=8 sts=4 sw=4 tw=0:
#
# Last Change: 15-Dec-2001.
# Written By:  Muraoka Taro <koron@tka.att.ne.jp>

use strict;
use Digest::MD5;

my $info = 'filelist.xml';
my $pkgname = '';
my $pkgurl = 'http://www.kaoriya.net/update/';

# ベースネームからテンプレートファイルを生成する。
sub gen_tmpname
{
    my $tmp = @_;
    while (-e $tmp)
    {
	$tmp .= ',tmp';
    }
    return $tmp;
}

# 名前で指定されたファイルのMD5値を計算する。
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

#my $str = &md5_file("chartable.txt");
#print "MD5:$str\n";

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
	next if $_ eq $info;
	my $name = $dir . $_;
	if (-d $name)
	{
	    push @$ls, @{&dirlist($name)};
	}
	else
	{
	    my $truename = $name;
	    my $comptype = 'none';
	    my $size = -s _;
	    my $md5;

	    # 圧縮されていた場合、復元してからMD5を計測
	    if ($truename =~ s/,bz2$//)
	    {
		$comptype = 'bz2';
		my $tmp = &gen_tmpname($truename);
		system "bzip2 -cdfk $name > $tmp";
		$md5 = &md5_file($tmp);
		unlink $tmp;
	    }
	    # 非圧縮ならば素のままMD5を計測
	    elsif ($comptype eq 'none')
	    {
		$md5 = &md5_file($name);
	    }

	    my $obj = {
		name=>$truename,
		size=>$size,
		md5=>$md5,
		compress=>$comptype,
	    };
	    push @$ls, $obj;
	    #printf "%7d %s %s\n", $obj->{size}, $obj->{md5}, $obj->{name};
	}
    }

    return $ls;
}

my $members = &dirlist('.');

# 既に情報ファイルがある場合は読み込んで$pkgnameと$pkgurlを取得する
if (-e  $info && open(XML, $info))
{
    while (<XML>)
    {
	if (m/<name>(.*)<\/name>/)
	{
	    $pkgname = $1;
	}
	elsif (m/<uri>(.*)<\/uri>/)
	{
	    $pkgurl = $1;
	}
    }
    close XML;
}

open XML, ">$info";
binmode XML;

printf XML <<"__END_of_XML__";
<?xml version="1.0" standalone="yes"?>
<!-- vim:set ts=4 sts=4 sw=4 tw=0: -->
<updateinfo>
\t<name>$pkgname</name>
\t<uri>$pkgurl</uri>
\t<filelist>
__END_of_XML__

foreach (@$members)
{
    if ($_->{compress} eq 'none')
    {
	printf XML "\t\t<file>\n";
    }
    else
    {
	printf XML "\t\t<file compress=\"$_->{compress}\">\n";
    }
    printf XML "\t\t\t<path>$_->{name}</path>\n";
    printf XML "\t\t\t<verify type=\"md5\">$_->{md5}</verify>\n";
    printf XML "\t\t\t<size>$_->{size}</size>\n";
    printf XML "\t\t</file>\n";
}

printf XML <<"__END_of_XML__";
\t</filelist>
</updateinfo>
__END_of_XML__

close XML;
