#!/usr/bin/perl

use strict;
use warnings;

my $in         = $ARGV[0];
my $codesout   = $ARGV[1];
my $codes      = $ARGV[1] =~ s/.*include\///gr;
my $protosout  = $ARGV[2];
my $protos     = $ARGV[2] =~ s/.*include\///gr;
my $asmout     = $ARGV[3];
my $asmcommon  = $ARGV[4];
my $prototypes = $ARGV[5] =~ s/.*include\///gr;
my $bits       = $ARGV[6];

my $codesdef   = $codes =~ tr/.-/_/r;
my $protosdef  = $protos =~ tr/.-/_/r;
my $code       = "code$bits";
my $need_aux   = 0;

unlink $codesout;
unlink $protosout;
unlink $asmout;

open CODESOUT,	">", $codesout	or die $!;
open PROTOSOUT, ">", $protosout or die $!;
open ASMOUT,	">", $asmout	or die $!;
open IN,	"<", $in	or die $!;

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

	my $code_macro;
	my $sys_name;

	if (/(?<name>\S+)\s+(?<alias>\S+)\s+(?<code64>\d+)\s+(?<code32>(?:\d+|\!))\s+\((?<args>.+)\)/) {
		$code_macro = "__NR_$+{name}";
		$sys_name   = "sys_$+{alias}";
	} elsif (/(?<name>\S+)\s+(?<code64>\d+)\s+(?<code32>(?:\d+|\!))\s+\((?<args>.+)\)/) {
		$code_macro = "__NR_$+{name}";
		$sys_name   = "sys_$+{name}";
	} else {
		unlink $codesout;
		unlink $protosout;
		unlink $asmout;

		die "Invalid syscall definition file: invalid entry $_\n";
	}

	if ($+{$code} ne "!") {
		print CODESOUT "#define $code_macro $+{$code}\n";

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

	print ASMOUT "syscall$nargs $sys_name, $code_macro\n";

	} else {
		$need_aux = 1;
	}

	print PROTOSOUT "extern long $sys_name($+{args});\n";
}

if ($need_aux == 1) {
	print ASMOUT   "#include \"asm/syscall-aux.S\"\n";
	print CODESOUT "#include \"asm/syscall-aux.h\"\n";
}

print CODESOUT  "#endif /* $codesdef */";
print PROTOSOUT "#endif /* $protosdef */";
