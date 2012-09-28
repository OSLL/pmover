#!/usr/bin/perl

use strict;
use warnings;

my $in         = $ARGV[0];
my $codesout   = $ARGV[1];
my $codes      = $ARGV[1] =~ s/include\///gr;
my $protosout  = $ARGV[2];
my $protos     = $ARGV[2] =~ s/include\///gr;
my $asmout     = $ARGV[3];
my $asmcommon  = $ARGV[4];
my $prototypes = $ARGV[5] =~ s/include\///gr;
my $bits       = $ARGV[6];

my $codesdef   = $codes =~ tr/.-/_/r;
my $protosdef  = $protos =~ tr/.-/_/r;
my $code       = "code$bits";

open CODESOUT, ">", $codesout;
open PROTOSOUT, ">", $protosout;
open ASMOUT, ">", $asmout;
open IN, "<", $in;

print CODESOUT <<"END";
/* Autogenerated, don't edit */
#ifndef $codesdef
#define $codesdef
END

print PROTOSOUT <<"END";
/* Autogenerated, don't edit */
#ifndef $protosdef
#define $protosdef
#include "$prototypes"
#include "$codes"
END

print ASMOUT <<"END";
/* Autogenerated, don't edit */
#include "$codes"
#include "$asmcommon"
END


for (<IN>) {
    if ($_ =~ /\#/) {
        next;
    }

    /(?<code_macro>\S+)\s+(?<code64>\d+)\s+(?<code32>\d+)\s+(?<sys_name>\S+)\s+\((?<args>.+)\)/;

    print CODESOUT "#define $+{code_macro} $+{$code}\n";
    print PROTOSOUT "extern long $+{sys_name}($+{args});\n";

    my $nargs;

    if ($+{args} eq "void") {
        $nargs = 0;
    } else {
        my $tmp = $+{args};
        $nargs = 1 + ($tmp =~ tr/\,/\,/);

        if ($nargs <= 4) {
            $nargs = 0;
        }
    }

    print ASMOUT "syscall$nargs $+{sys_name}, $+{$code}\n";
}

print CODESOUT  "#endif /* $codesdef */";
print PROTOSOUT "#endif /* $protosdef */";