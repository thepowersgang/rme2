<?php
#if 0
// Voodoo to preprocess the file before PHP gets it
$exename = $argv[0];
$tmpname = ".".basename($exename).".tmp";
if(dirname($exename) != "")
	$tmpname = dirname($exename)."/".$tmpname;
$args = array_splice($argv, 1);
exec("cpp -P ".$exename." -o ".$tmpname);
exec("php ".$tmpname." > mainswitch.c");
unlink($tmpname);
exit;
__halt_compiler();
#endif

// === Operation Names ===
$casRegsD = array("EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI");
$casRegsW = array("AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI");
$casRegsB = array("AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH");
$casArithOps = array("ADD", "OR", "ADC", "SBB", "AND", "SUB", "XOR", "CMP");

// Functions
// --- Helpers ---
function SetCommonFlags($value, $width)
{
	echo "\t\tState->Flags |= ((",$value,") == 0) ? FLAG_ZF : 0;\n";
	echo "\t\tState->Flags |= ((",$value,") >> (",($width-1),")) ? FLAG_SF : 0;\n";
	
	switch($width)
	{
	case 8:
		echo "\t\tState->Flags |= PAIRITY8($value) ? FLAG_PF : 0;\n";
		break;
	case 16:
		echo "\t\tState->Flags |= PAIRITY16($value) ? FLAG_PF : 0;\n";
		break;
	default:
		echo "\t\tState->Flags |= (PAIRITY16($value) ^ PAIRITY16(($value)>>16)) ? FLAG_PF : 0;\n";
		break;
	}
}

// --- Arithmatic Functions ---
// 0: Add
function DoAdd($to, $from, $width)
{
	echo "\t\t($to) += ($from);\n";
	echo "\t\tState->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\n";
	SetCommonFlags( $to, $width );
	echo "\t\tState->Flags |= ($to) < ($from) ? FLAG_OF|FLAG_CF : 0\n";
}
// 1: Bitwise OR
function DoOr ($to, $from, $width)
{
	echo "\t\t($to) |= ($from);\n";
	echo "\t\tState->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\n";
	SetCommonFlags( $to, $width );
}
// 2: Add with carry
function DoAdc($to, $from, $width)
{
	echo "\t\t($to) += ($from) + ((State->Flags&FLAG_CF)?1:0);\n";
	echo "\t\tState->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\n";
	SetCommonFlags( $to, $width );
	echo "\t\tState->Flags |= (($to) < ($from)) ? FLAG_OF|FLAG_CF : 0;\n";
}
// 3: Subtract with borrow
function DoSbb($to, $from, $width)
{
	echo "\t\t($to) -= ($from) + ((State->Flags&FLAG_CF)?1:0);\n";
	echo "\t\tState->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\n";
	SetCommonFlags( $to, $width );
	echo "\t\tState->Flags |= (($to) > ($from)) ? FLAG_OF|FLAG_CF : 0;\n";
}
// 4: Bitwise AND
function DoAnd($to, $from, $width)
{
	echo "\t\t($to) &= ($from);\n";
	echo "\t\tState->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\n";
	SetCommonFlags( $to, $width );
}
// 5: Subtract
function DoSub($to, $from, $width)
{
	echo "\t\t($to) -= ($from);\n";
	echo "\t\tState->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\n";
	SetCommonFlags( $to, $width );
	echo "\t\tState->Flags |= (($to) > ($from)) ? FLAG_OF|FLAG_CF : 0;\n";
}
// 6: Bitwise XOR
function DoXor($to, $from, $width)
{
	echo "\t\t(to) ^= (from);\n";
	echo "\t\tState->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\n";
	SetCommonFlags( $to, $width );
}
// 7: Compare (Set flags according to SUB)
function DoCmp($to, $from, $width)
{
	echo "\t\tint v = ($to)-($from);\n";
	echo "\t\tState->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);\n";
	SetCommonFlags( "v", $width );
	echo "\t\tState->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;\n";
}

// Arithmatic Switcher
function DoArithOp( $num, $to, $from, $width )
{
	switch($num)
	{
	case 0:	DoAdd($to, $from, $width);	break;
	case 1:	DoOr ($to, $from, $width);	break;
	case 2:	DoAdc($to, $from, $width);	break;
	case 3:	DoSbb($to, $from, $width);	break;
	case 4:	DoAnd($to, $from, $width);	break;
	case 5:	DoSub($to, $from, $width);	break;
	case 6:	DoXor($to, $from, $width);	break;
	case 7:	DoCmp($to, $from, $width);	break;
	}
}


function DoCondJMP($type, $ofsVar, $name)
{
	echo "\t\tDEBUG_S(\"J\");"
	switch($type) {
	case 0x0:
		echo "\t\tDEBUG_S(\"O\");	// Overflow\n";
		echo "\t\tif(State->Flags & FLAG_OF)\n";
		break;
	case 0x1:
		echo "\t\tDEBUG_S(\"NO\");	// No Overflow\n";
		echo "\t\tif(!(State->Flags & FLAG_OF))\n"
		break;
	case 0x2:
		echo "\t\tDEBUG_S(\"C\");	// Carry\n";
		echo "\t\tif(State->Flags & FLAG_CF)\n";
		break;
	case 0x3:
		echo "\t\tDEBUG_S(\"NC\");	// No Carry\n"
		echo "\t\tif(!(State->Flags & FLAG_CF))\n"
		break;
	case 0x4:
		echo "\t\tDEBUG_S(\"Z\");	// Equal\n"
		echo "\t\tif(State->Flags & FLAG_ZF)\n"
		break;
	case 0x5:
		echo "\t\tDEBUG_S(\"NZ\");	// Not Equal\n";
		echo "\t\tif(!(State->Flags & FLAG_ZF))\n";
		break;
	case 0x6:
		echo "\t\tDEBUG_S(\"BE\");	// Below or Equal\n";
		echo "\t\tif(State->Flags & FLAG_CF || State->Flags & FLAG_ZF)\n";
		break;
	case 0x7:
		echo "\t\tDEBUG_S(\"A\");	// Above\n"
		echo "\t\tif( !(State->Flags & FLAG_CF) && !(State->Flags & FLAG_ZF))\n";
		break;
	case 0x8:
		echo "\t\tDEBUG_S(\"S");	// Sign\n"
		echo "\t\tif( State->Flags & FLAG_SF )\n"
		break;
	case 0x9:
		echo "\t\tDEBUG_S(\"NS\");	// Not Sign\n"
		break;
	case 0xA:
		echo "\t\tDEBUG_S(\"PE\");	// Pairity Even\n";
		break;
	case 0xB:
		echo "\t\tDEBUG_S(\"PO\");	// Pairity Odd\n";
		echo "\t\tif( !(State->Flags & FLAG_PF) )\n";
		break;
	case 0xC:
		echo "\t\tDEBUG_S(\"L\");	// Less\n";
		echo "\t\tif( !!(State->Flags & FLAG_SF) != !!(State->Flags & FLAG_OF) )\n";
		break;
	case 0xD:
		echo "\t\tDEBUG_S(\"GE\");	// Greater or Equal\n"
		echo "\t\tif( !!(State->Flags & FLAG_SF) == !!(State->Flags & FLAG_OF) )\n";
		break;
	case 0xE:
		echo "\t\tDEBUG_S(\"LE\");	// Less or Equal\n";
		echo "\t\tif( State->Flags & FLAG_ZF || !!(State->Flags & FLAG_SF) != !!(State->Flags & FLAG_OF) )\n";
		break;
	case 0xF:
		echo "\t\tDEBUG_S(\"G\");	// Greater\n";
		echo "\t\tif( !(State->Flags & FLAG_ZF) || !!(State->Flags & FLAG_SF) == !!(State->Flags & FLAG_OF) )\n";
		break;

	default:
		echo "#error Unknow jmp code $type\n";
		return RME_ERR_BUG;
	}
	echo "\t\t	State->IP += $ofsVar;\n";
	echo "\t\tDEBUG_S(\" $name .+0x%04x\", $ofsVar);\n";
	return 0;
}

// CASEK(num, start, step) - $case is the value
#define CASEK(_num, _start, _step)	for($i=0,$case=(_start);$i<(_num);$i++,$case+=(_step))

// <op> MR
CASEK(8, 0x00, 0x8) {
	echo "\tcase $case:\n";
	echo "\t\tDEBUG_S(\"",$casArithOps[$i]," (MR)\");\n";
	echo "\t\tret = RME_Int_ParseModRM(State, &fromB, &toB);\n";
	echo "\t\tif(ret)	return ret;\n";
	DoArithOp( $case / 8, "*toB", "*fromB", 8 );
	echo "\t\tbreak;\n";
	echo "\n";
}
// <op> MRX
CASEK(8, 0x01, 0x8) {
	echo "\tcase $case:\n";
	echo "\t\tDEBUG_S(\"",$casArithOps[$i]," (MRX)\");\n";
	echo "\t\tret = RME_Int_ParseModRMX(State, &from.W, &to.W);\n";
	echo "\t\tif(ret)	return ret;\n";
	echo "\t\tif(State->Decoder.bOverrideOperand){\n";
	DoArithOp( $case >> 3, "*to.D", "*from.D", 32 );
	echo "\t\t} else {\n";
	DoArithOp( $case >> 3, "*to.W", "*from.W", 16 );
	echo "\t\t}\n";
	echo "\t\tbreak;\n";
	echo "\n";
}
// <op> RM
CASEK(8, 0x02, 0x8) {
	echo "\tcase $case:\n";
	echo "\t\tDEBUG_S(\"",$casArithOps[$i]," (RM)\");\n";
	echo "\t\tret = RME_Int_ParseModRM(State, &toB, &fromB);\n";
	echo "\t\tif(ret)	return ret;\n";
	DoArithOp( $case >> 3, "*toB", "*fromB", 8 );
	echo "\t\tbreak;\n";
	echo "\n";
}
// <op> RMX
CASEK(8, 0x03, 0x8) {
	echo "\tcase $case:\n";
	echo "\t\tDEBUG_S(\"",$casArithOps[$i]," (RM)\");\n";
	echo "\t\tret = RME_Int_ParseModRM(State, &toB, &fromB);\n";
	echo "\t\tif(ret)	return ret;\n";
	echo "\t\tif(State->Decoder.bOverrideOperand){\n";
	DoArithOp( $case >> 3, "*to.D", "*from.D", 32 );
	echo "\t\t} else {\n";
	DoArithOp( $case >> 3, "*to.W", "*from.W", 16 );
	echo "\t\t}\n";
	echo "\t\tbreak;\n";
	echo "\n";
}

// <op> AI
CASEK(8, 0x04, 8) {
	echo "\tcase $case:\n";
	echo "\t\tREAD_INSTR8( pt2 );\n";
	echo "\t\tDEBUG_S(\"",$casArithOps[$i]," (AI) AL 0x%02x\", pt2);\n";
	DoArithOp( $case >> 3, "State->AX.B.L", "pt2", 8 );
	echo "\t\tbreak;\n";
	echo "\n";
}
// <op> AIX
CASEK(8, 0x05, 8) {
	echo "\tcase $case:\n";
	echo "\t\tif(State->Decoder.bOverrideOperand) {\n";
	echo "\t\tREAD_INSTR32( dword );\n";
	echo "\t\tDEBUG_S(\"",$casArithOps[$i]," (AIX) EAX 0x%08x\", dowrd);\n";
	DoArithOp( $case >> 3, "State->AX.D", "dowrd", 32 );
	echo "\t\t} else {\n";
	echo "\t\tREAD_INSTR16( pt2 );\n";
	echo "\t\tDEBUG_S(\"",$casArithOps[$i]," (AIX) AX 0x%04x\", pt2);\n";
	DoArithOp( $case >> 3, "State->AX.W", "pt2", 16 );
	echo "\t\t}\n";
	echo "\t\tbreak;\n";
	echo "\n";
}


// INC Register
CASEK(8, 0x40, 1) {
	echo "\tcase $case:\n";
	echo "\t\tif(State->Decoder.bOverrideOperand) {\n";
	echo "\t\t\tDEBUG_S(\"INC ",$casRegsD[$case&7],"\");\n";
	echo "\t\t\tto.D = &State->",$casRegsW[$case&7],".D;\n";
	echo "\t\t\tState->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);\n";
	echo "\t\t\t(*to.D) ++;\n";
	SetCommonFlags("*to.D", 32);
	echo "\t\t} else {\n";
	echo "\t\t\tDEBUG_S(\"INC ",$casRegsW[$case&7],"\");\n";
	echo "\t\t\tto.W = &State->",$casRegsW[$case&7],".W;\n";
	echo "\t\t\tState->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);\n";
	echo "\t\t\t(*to.W) ++;\n";
	SetCommonFlags("*to.W", 16);
	echo "\t\t}\n";
	echo "\t\tState->Flags |= (State->Flags & FLAG_ZF) ? FLAG_OF : 0;\n";
	echo "\t\tbreak;\n";
	echo "\n";
	break;
}
// DEC Register
CASEK(8, 0x48, 1) {
	echo "\tcase $case:\n";
	echo "\t\tif(State->Decoder.bOverrideOperand) {\n";
	echo "\t\t\tDEBUG_S(\"DEC ",$casRegsD[$case&7],"\");\n";
	echo "\t\t\tto.D = &State->",$casRegsW[$case&7],".D;\n";
	echo "\t\t\tState->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);\n";
	echo "\t\t\t(*to.D) --;\n";
	SetCommonFlags("*to.D", 32);
	echo "\t\t\tState->Flags |= (*to.D == 0xFFFFFFFF) ? FLAG_OF : 0;\n";
	echo "\t\t} else {\n";
	echo "\t\t\tDEBUG_S(\"DEC ",$casRegsW[$case&7],"\");\n";
	echo "\t\t\tto.W = &State->",$casRegsW[$case&7],".W;\n";
	echo "\t\t\tState->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);\n";
	echo "\t\t\t(*to.W) --;\n";
	SetCommonFlags("*to.W", 16);
	echo "\t\t\tState->Flags |= (*to.W == 0xFFFF) ? FLAG_OF : 0;\n";
	echo "\t\t}\n";
	echo "\t\tbreak;\n";
	echo "\n";
	break;
}

// MOV RI
CASEK(8, 0xB0, 1) {
	echo "\tcase $case:\n";
	echo "\t\tDEBUG_S(\"MOV (RI) ",$casRegsB[$case&7],"\");\n";
	echo "\t\tREAD_INSTR8( pt2 );\n";
	echo "\t\tDEBUG_S(\" 0x%02x\", pt2);\n";
	echo "\t\tState->",$casRegsW[$case&7],".B.L = pt2;\n";
	echo "\t\tbreak;\n";
	echo "\n";
}
// MOV RIX
CASEK(8, 0xB8, 1) {
	echo "\tcase $case:\n";
	echo "\t\tif(State->Decoder.bOverrideOperand) {\n";
	
	echo "\t\tDEBUG_S(\"MOV (RIX) ",$casRegsD[$case&7],"\");\n";
	echo "\t\tREAD_INSTR32( dword );\n";
	echo "\t\tDEBUG_S(\" 0x%08x\", dword);\n";
	echo "\t\tState->",$casRegsW[$case&7],".D = dword;\n";
	
	echo "\t\t} else {\n";
	
	echo "\t\tDEBUG_S(\"MOV (RIX) ",$casRegsW[$case&7],"\");\n";
	echo "\t\tREAD_INSTR16( pt2 );\n";
	echo "\t\tDEBUG_S(\" 0x%04x\", pt2);\n";
	echo "\t\tState->",$casRegsW[$case&7],".W = pt2;\n";
	
	echo "\t\t}\n";
	echo "\t\tbreak;\n";
	echo "\n";
}
// XCHG - Exchange AX with Register (Ignores AX,AX)
CASEK(8-1, 0x90+1, 1) {
	echo "\tcase $case:\n";
	echo "\t\tif(State->Decoder.bOverrideOperand)	DEBUG_S(\"XCHG EAX\");\n";
	echo "\t\telse	DEBUG_S(\"XCHG AX\");\n";
	echo "\t\tif(State->Decoder.bOverrideOperand)\n";
	echo "\t\t\tXCHG(State->AX.D, State->",$casRegsW[$case&7],".D);\n";
	echo "\t\telse\n";
	echo "\t\t\tXCHG(State->AX.W, State->",$casRegsW[$case&7],".W);\n";
	echo "\t\tbreak;";
}

// Conditional Jump
CASEK(16, 0x70, 1) {
	echo "\tcase $case:\n";
	echo "\t\tREAD_INSTR8S( pt2 );\n";
	DoCondJMP($case, "pt2", "(S)");
	echo "\t\tbreak;";
}

?>
