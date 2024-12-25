#@ Automated test for S-bsdipa (make test).

#use Test::Simple tests => 1;
use Test2::API qw/context/;
our @EXPORT = qw/ok done_testing/;
sub ok($;$){
	my ($bool, $name) = @_;
	my $ctx = context();
	$ctx->ok($bool, $name);
	$ctx->release;
	return $bool
}
sub done_testing{
	my $ctx = context();
	$ctx->done_testing;
	$ctx->release
}

BEGIN{
	require BsDiPa;
}

use strict;
use diagnostics;
use Compress::Zlib;

## Core:

use BsDiPa;

#print BsDiPa::VERSION, "\n";
#print BsDiPa::CONTACT, "\n";
#print BsDiPa::COPYRIGHT;

my ($b, $a) = ("\012\013\00\01\02\03\04\05\06\07" x 3, "\010\011\012\013\014" x 4);

my $pz;
ok(BsDiPa::core_diff_zlib(undef, $a, \$pz) eq BsDiPa::INVAL);
ok(BsDiPa::core_diff_zlib($b, undef, \$pz) eq BsDiPa::INVAL);
ok(BsDiPa::core_diff_zlib($b, $a, undef) eq BsDiPa::INVAL);
ok(BsDiPa::core_diff_zlib($b, $a, $pz) eq BsDiPa::INVAL);
ok(BsDiPa::core_diff_zlib($b, $a, \$pz) eq BsDiPa::OK);

my $pr;
ok(BsDiPa::core_diff_raw(undef, $a, \$pr) eq BsDiPa::INVAL);
ok(BsDiPa::core_diff_raw($b, undef, \$pr) eq BsDiPa::INVAL);
ok(BsDiPa::core_diff_raw($b, $a, undef) eq BsDiPa::INVAL);
ok(BsDiPa::core_diff_raw($b, $a, $pr) eq BsDiPa::INVAL);
ok(BsDiPa::core_diff_raw($b, $a, \$pr) eq BsDiPa::OK);

my $x = uncompress($pz);
ok(($pr cmp $x) == 0);




######FIXME
open X, '<', '/tmp/x/0';
binmode X;
read X, $b, 2000000000 or die "UAUAU: $^E";
close X;
open X, '<', '/tmp/x/1';
binmode X;
read X, $a, 2000000000 or die "UAUAU 2: $^E";
close X;

ok(BsDiPa::core_diff_zlib($b, $a, \$pz) eq BsDiPa::OK);
	open X, '>', '/tmp/x/p.perl';
	binmode X or die "f1-binmode";
	print X $pz;
	binmode X or die "f1-flush";


done_testing()
# s-itt-mode
