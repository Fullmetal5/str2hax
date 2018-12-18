#!/usr/bin/perl


# Read PPM file.

$sig = <>; chomp $sig;
$sizes = <>; chomp $sizes;
$cols = <>; chomp $cols;

{
	local $/;
	$data = <>;
}


# Sanity check.

$sig ne "P6" and die;
$sizes ne "90 256" and die;
$cols ne "255" and die;
(length $data) != 3 * 90 * 256 and die;


# Output header.

print "// GENERATED FILE DO NOT EDIT\n";
print "\n";
print "#include \"loader.h\"\n";
print "\n";
print "const u8 console_font_10x16x4[96*80] = {\n";

# Output data.

for my $ch (2..7) {
	for my $cl (0..15) {
		printf "\n\t// %x%x\n", $ch, $cl;
		for my $py (0..15) {
			print "\t";
			for my $px (0..9) {
				my $hor = $px + 10*($ch - 2);
				my $ver = $py + 16*$cl;
				my $wot = $hor + 90*$ver;
				my $bytes = substr($data, 3*$wot, 3);
				my $nyb = int ((ord $bytes) / 16);
				if (($px & 1) == 0) {
					printf "0x%x", $nyb;
				} else {
					printf "%x,", $nyb;
				}
			}
			print "\n";
		}
	}
}


# Output footer.

print "\n";
print "};\n";
