#!/usr/bin/perl 

use strict;
use warnings;
use feature ':5.30';
use Modern::Perl '2020';

my $ftdiExe = '/usr/local/bin/ft232r_prog';
my $ftdiDump = "$ftdiExe --dump";
my $ftdiInvert = "$ftdiExe --invert_rxd";

my $dump = `$ftdiDump`;

#say "$dump";

my ($rxInverted) = $dump =~ /rxd_inverted\s=\s(\d)/;

#say "rxInverted = $rxInverted";

if ($rxInverted != 0) {
    say "already inverted, exiting";
    exit 0;
}

`$ftdiInvert`;
say "inversion done";
