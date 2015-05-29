#!/usr/bin/perl

my $addr,$opcode,$inst,$args;
sub parse {
    local ($l)=@_;
    $addr=$opcode=$inst=$args="";
    if ($l =~ /^\s*([0-9a-f]*):\s*([0-9a-f][0-9a-f] [0-9a-f][0-9a-f] [0-9a-f][0-9a-f] [0-9a-f][0-9a-f])\s*([^\s]*)\s*(.*)$/) {
	$addr=$1;
	$opcode=$2;
	$inst=$3;
	$args=$4;
	chomp $args;
    }
}


my $globError=0;


sub test_AT697E {




    printf ("AT697E processor errata checks review.\n\n");
    printf ("File: [$XMP/core/xm_core].\n");
    printf ("Number of lines reviewedd: [$nlines].\n\n");

    printf "Step 1: Floating point instructions shall only be used for saving and restoring the context of the FPU?\n";
    $error=0;
    foreach $line (@ASM) {
	if ($line =~ /^[0-9a-f]*\s<([^>]*)>/) {
	    $function= $1;
	}

	#print "$line $function ";
	parse($line);


	if (($inst  =~ /^ld/) && ($args =~ /%f[0-9]/) ){
	    printf "+";
	    if (not $function =~ /FpDisabledTrap/) {
		printf("\t%-16s %s", "$function:" ,  $line);
		$error=1;
	    }
	} elsif (($inst  =~ /^st/) && ($args =~ /%f[0-9]/) ){
	    printf "-";
	    if (not $function =~ /FpDisabledTrap/) {
		printf("\t%-16s %s", "$function:",  $line);
		$error=1;
	    }
	} elsif ($inst =~ /^ldf/){
	    printf("\t%-16s %s", "$function:",  $line); $error=1;
	} elsif ($inst =~ /^fad/){
	    printf("\t%-16s %s", "$function:",  $line); $error=1;
	} elsif ($inst =~ /fmul/){
	    printf("\t%-16s %s", "$function:",  $line); $error=1;
	} elsif ($inst =~ /fsrq/){
	    printf("\t%-16s %s", "$function:",  $line); $error=1;
	} elsif ($inst =~ /fsto/){
	    printf("\t%-16s %s", "$function:",  $line); $error=1;
	} elsif ($inst =~ /fdiv/){
	    printf("\t%-16s %s", "$function:",  $line); $error=1;
	} 
    }
    $globError += $error;
    if ($error) {
	printf "\n\t!ERROR!.\n";
	printf "\tSystem code shall not use the PFU unit!. See the errata List of the AT697E processor\n\n";
    } else {
	printf " => Correct.\n";
    }

    printf "Step 2: Uses SDIVCC or UDIVCC instructions? ";
    $error=0;
    foreach $line (@ASM) {
	
	if ($line =~ /^[0-9a-f]*\s<([^>]*)>/) {
	    $function= $1;
	}

	parse($line);

	if ($inst =~ /sdiv/){
	    printf("%-16s %s", "$function:" ,  $line);
	    $error=1;
	} elsif  ($inst =~ /udiv/){
	    printf("%-16s %s", "$function:" ,  $line);
	    $error=1;
	}
    }
    $globError += $error;
    if ($error) {
	printf "\n\tERROR!.\n\tSee the errata List of the AT697E processor\n\n";

    } else {
	printf "\n\tNo => Correct.\n";
    }

    printf "Step 3: Call Return Address Failure with Large Displacement.";
    $error=0;
    $backward=0; $bf="";
    $forward=0;  $ff="";
    foreach $line (@ASM) {
	
	if ($line =~ /^[0-9a-f]*\s<([^>]*)>/) {
	    $function= $1;
	}
	
	parse($line);
	
	if ($inst =~ /call/){
	    #printf("%-6s %-35s", $inst, $args);
	    if ($args =~ /^([0-9a-f]+)/){
		$res= hex ("$1") - hex("$addr") ;
		#printf("%10s %s %s $line", "[$res]", "<$addr>", "{$1}");
		$forward =$res, $ff=$function."@ 0x".$addr if ($forward < $res );
		$backward=$res, $bf=$function."@ 0x".$addr if ($res < $backward);
	    }
	    #print "\n";
	}
    }

    printf "\n\tLongest forward call:  %10d < 16Mb   at $ff\n", $forward;
    printf "\tLongest backward call: %10d > -2Mb   at $bf\n", $backward;


    if ( ($backward<-(1024*1024*2)) || ($forward>(1024*1024*16)) ) {
	printf ("\tERROR. See the errata List of the AT697E processor\n");
	$error=1;
    } else {
	printf "\tThen => Correct (Note: only immedite calls has been checked)\n";
    }


    $globError += $error;


    exit -$globError;
}

sub findMuls{
    printf("Multiplication and division operations list.\n");

    printf("All the instructions that use or call the integer mul/div are listed\n\n");
    printf ("File: [$XMP/core/xm_core].\n");
    printf ("Number of lines reviewedd: [$nlines].\n\n");
    printf("%-14s %-10s %-12s %-10s %s\n",, "Function", "Address:", "Opcode:", "Code:" , "Args:");
    foreach $line (@ASM) {
	if ($line =~ /^[0-9a-f]*\s<([^>]*)>/) {
	    $function= $1;
	}
	
	parse($line);
	if (($args =~ /\.[u]div/) ||
	    ($args =~ /[^E]mul/)  ||
	    ($inst =~ /mul/)){
	    if (not $function =~ /(mul_s)|(__muld)|(\.umul)|(udivd)|(modd)/) {
		printf("%-14s %-10s %-12s %-10s %s\n", $function, "$addr:", $opcode, $inst, $args );
	    }
	}
    }
}


##################### main #################
$XMP = $ENV{'XTRATUM_PATH'};
$option= @ARGV[0];

if ($option =~ /-md/ ){
    $binary=$ARGV[1];
} else {  
    $binary=$ARGV[0];  
}
if (not $binary){
    $binary="$XMP/core/xm_core";
}
if (!$XMP) {
    printf "XTRATUM_PATH not set!.\n";
    exit -1;
}


$definition = `file $binary`;

if (not $definition =~ /.*ELF.*SPARC/){
    printf("File $XMP/core/xm_core is not an SPARC executable\n");
    exit -1;
}

@ASM = `. $XMP/xmconfig; \${OBJDUMP} -d $binary`;
$nlines = @ASM;





if ($option =~ /-md/) {
    findMuls();
} else {
    test_AT697E();
}
