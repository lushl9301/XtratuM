#!/usr/bin/perl

my $USE="esymb.pl <xm_core> <symb_list>\n";

if ($#ARGV!=1) {
    die $USE;
}

my $NM="nm";
my $GREP="grep";

my $xm_core=$ARGV[0];
my $symbols=$ARGV[1];
my $line=0;

open SYMB_FILE , "<", $symbols or die $!; 

$cmd=$NM." -s ".$xm_core." 2>/dev/null";
$symbList=`$cmd`;
if ( $? != 0) {
    print "Error processing \"".$xm_core."\"\n";
    exit(-1);
}


@symbList = split('\n', $symbList);

#foreach (@symbList) {
#    $startMod=$_;
#    if ($startMod =~ s/([0-9a-fA-F]{8}) [A-Za-z?] startModAddr/0x$1/) {
#        last;
#    }
#}

while (<SYMB_FILE>) {
    $line++;
    $s=$_;
    if ($s =~ m/^#/) { # Comment
        next;
    }

    if (($s =~ s/([a-zA-Z_]\w+)\n/$1/)|| ($s =~ s/([a-zA-Z_]\w+)/$1/)) {
        foreach (@symbList) {
            $symbAddr=$_;
            if ($symbAddr =~ s/([0-9a-fA-F]{8}) [A-Za-z?] ($s)/$2 = 0x$1;/) {
                push(@symbTab, $symbAddr);
            }
        }
        next;
    }

    if ($s =~ m/^\n/) {      
        next;
    }

    print "(Line ".$line.") Unexpected expression \"".$_."\"\n";
    exit -2;
}

$symbList=join("\n    ", @symbTab);

$fileLds=sprintf(
"
#include <comp.h>
#include <module.h>

#define XM_SET_VERSION(_ver, _subver, _rev) ((((_ver)&0xFF)<<16)|(((_subver)&0xFF)<<8)|((_rev)&0xFF))

ENTRY(modStAddr);
SECTIONS
{
    %s

    . = modStAddr;

    .modHdr ALIGN(8): {
        LONG(XM_MODULE_SIGNATURE);
        LONG(XM_SET_VERSION(CONFIG_XM_VERSION, CONFIG_XM_SUBVERSION, CONFIG_XM_REVISION));
        LONG(BUILD_TIME);
        LONG(BUILD_IDR);
        LONG(modStAddr);
        LONG(_endMod - modStAddr);
        LONG(exPTable);
        *(.iModHdr);
        LONG(0);
        *(.modStr);
    }

    .text ALIGN(8): {
        *(.text)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(8);
        exPTable = .;
        *(.exptable)
        LONG(0);
        LONG(0);
    }

    .data ALIGN(8): {
        *(.data)                
    }

    .bss ALIGN(8): {
        *(COMMON)
        *(.bss)
    }
    _endMod = .;
}
", $symbList);

print $fileLds;
#$symbList=join("\n", @symbTab);

#print $symbList;

close SYMB_FILE;
