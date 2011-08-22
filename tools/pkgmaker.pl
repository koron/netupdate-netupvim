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

# �x�[�X�l�[������e���v���[�g�t�@�C���𐶐�����B
sub gen_tmpname
{
    my $tmp = @_;
    while (-e $tmp)
    {
	$tmp .= ',tmp';
    }
    return $tmp;
}

# ���O�Ŏw�肳�ꂽ�t�@�C����MD5�l���v�Z����B
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

	    # ���k����Ă����ꍇ�A�������Ă���MD5���v��
	    if ($truename =~ s/,bz2$//)
	    {
		$comptype = 'bz2';
		my $tmp = &gen_tmpname($truename);
		system "bzip2 -cdfk $name > $tmp";
		$md5 = &md5_file($tmp);
		unlink $tmp;
	    }
	    # �񈳏k�Ȃ�Αf�̂܂�MD5���v��
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

# ���ɏ��t�@�C��������ꍇ�͓ǂݍ����$pkgname��$pkgurl���擾����
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
