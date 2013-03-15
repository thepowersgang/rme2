	case 0:
		DEBUG_S("ADD (MR)");
		ret = RME_Int_ParseModRM(State, &fromB, &toB);
		if(ret)	return ret;
		(*toB) += (*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		State->Flags |= (*toB) < (*fromB) ? FLAG_OF|FLAG_CF : 0
		break;

	case 8:
		DEBUG_S("OR (MR)");
		ret = RME_Int_ParseModRM(State, &fromB, &toB);
		if(ret)	return ret;
		(*toB) |= (*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		break;

	case 16:
		DEBUG_S("ADC (MR)");
		ret = RME_Int_ParseModRM(State, &fromB, &toB);
		if(ret)	return ret;
		(*toB) += (*fromB) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		State->Flags |= ((*toB) < (*fromB)) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 24:
		DEBUG_S("SBB (MR)");
		ret = RME_Int_ParseModRM(State, &fromB, &toB);
		if(ret)	return ret;
		(*toB) -= (*fromB) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		State->Flags |= ((*toB) > (*fromB)) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 32:
		DEBUG_S("AND (MR)");
		ret = RME_Int_ParseModRM(State, &fromB, &toB);
		if(ret)	return ret;
		(*toB) &= (*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		break;

	case 40:
		DEBUG_S("SUB (MR)");
		ret = RME_Int_ParseModRM(State, &fromB, &toB);
		if(ret)	return ret;
		(*toB) -= (*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		State->Flags |= ((*toB) > (*fromB)) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 48:
		DEBUG_S("XOR (MR)");
		ret = RME_Int_ParseModRM(State, &fromB, &toB);
		if(ret)	return ret;
		(to) ^= (from);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		break;

	case 56:
		DEBUG_S("CMP (MR)");
		ret = RME_Int_ParseModRM(State, &fromB, &toB);
		if(ret)	return ret;
		int v = (*toB)-(*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((v) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((v) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(v) ? FLAG_PF : 0;
		State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 1:
		DEBUG_S("ADD (MRX)");
		ret = RME_Int_ParseModRMX(State, &from.W, &to.W);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) += (*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= (*to.D) < (*from.D) ? FLAG_OF|FLAG_CF : 0
		} else {
		(*to.W) += (*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		State->Flags |= (*to.W) < (*from.W) ? FLAG_OF|FLAG_CF : 0
		}
		break;

	case 9:
		DEBUG_S("OR (MRX)");
		ret = RME_Int_ParseModRMX(State, &from.W, &to.W);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) |= (*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		} else {
		(*to.W) |= (*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		}
		break;

	case 17:
		DEBUG_S("ADC (MRX)");
		ret = RME_Int_ParseModRMX(State, &from.W, &to.W);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) += (*from.D) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= ((*to.D) < (*from.D)) ? FLAG_OF|FLAG_CF : 0;
		} else {
		(*to.W) += (*from.W) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		State->Flags |= ((*to.W) < (*from.W)) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 25:
		DEBUG_S("SBB (MRX)");
		ret = RME_Int_ParseModRMX(State, &from.W, &to.W);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) -= (*from.D) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= ((*to.D) > (*from.D)) ? FLAG_OF|FLAG_CF : 0;
		} else {
		(*to.W) -= (*from.W) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		State->Flags |= ((*to.W) > (*from.W)) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 33:
		DEBUG_S("AND (MRX)");
		ret = RME_Int_ParseModRMX(State, &from.W, &to.W);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) &= (*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		} else {
		(*to.W) &= (*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		}
		break;

	case 41:
		DEBUG_S("SUB (MRX)");
		ret = RME_Int_ParseModRMX(State, &from.W, &to.W);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) -= (*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= ((*to.D) > (*from.D)) ? FLAG_OF|FLAG_CF : 0;
		} else {
		(*to.W) -= (*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		State->Flags |= ((*to.W) > (*from.W)) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 49:
		DEBUG_S("XOR (MRX)");
		ret = RME_Int_ParseModRMX(State, &from.W, &to.W);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(to) ^= (from);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		} else {
		(to) ^= (from);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		}
		break;

	case 57:
		DEBUG_S("CMP (MRX)");
		ret = RME_Int_ParseModRMX(State, &from.W, &to.W);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		int v = (*to.D)-(*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((v) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((v) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(v) ^ PAIRITY16((v)>>16)) ? FLAG_PF : 0;
		State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;
		} else {
		int v = (*to.W)-(*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((v) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((v) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(v) ? FLAG_PF : 0;
		State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 2:
		DEBUG_S("ADD (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		(*toB) += (*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		State->Flags |= (*toB) < (*fromB) ? FLAG_OF|FLAG_CF : 0
		break;

	case 10:
		DEBUG_S("OR (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		(*toB) |= (*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		break;

	case 18:
		DEBUG_S("ADC (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		(*toB) += (*fromB) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		State->Flags |= ((*toB) < (*fromB)) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 26:
		DEBUG_S("SBB (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		(*toB) -= (*fromB) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		State->Flags |= ((*toB) > (*fromB)) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 34:
		DEBUG_S("AND (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		(*toB) &= (*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		break;

	case 42:
		DEBUG_S("SUB (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		(*toB) -= (*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		State->Flags |= ((*toB) > (*fromB)) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 50:
		DEBUG_S("XOR (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		(to) ^= (from);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*toB) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*toB) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(*toB) ? FLAG_PF : 0;
		break;

	case 58:
		DEBUG_S("CMP (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		int v = (*toB)-(*fromB);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((v) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((v) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(v) ? FLAG_PF : 0;
		State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 3:
		DEBUG_S("ADD (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) += (*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= (*to.D) < (*from.D) ? FLAG_OF|FLAG_CF : 0
		} else {
		(*to.W) += (*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		State->Flags |= (*to.W) < (*from.W) ? FLAG_OF|FLAG_CF : 0
		}
		break;

	case 11:
		DEBUG_S("OR (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) |= (*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		} else {
		(*to.W) |= (*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		}
		break;

	case 19:
		DEBUG_S("ADC (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) += (*from.D) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= ((*to.D) < (*from.D)) ? FLAG_OF|FLAG_CF : 0;
		} else {
		(*to.W) += (*from.W) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		State->Flags |= ((*to.W) < (*from.W)) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 27:
		DEBUG_S("SBB (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) -= (*from.D) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= ((*to.D) > (*from.D)) ? FLAG_OF|FLAG_CF : 0;
		} else {
		(*to.W) -= (*from.W) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		State->Flags |= ((*to.W) > (*from.W)) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 35:
		DEBUG_S("AND (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) &= (*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		} else {
		(*to.W) &= (*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		}
		break;

	case 43:
		DEBUG_S("SUB (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(*to.D) -= (*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= ((*to.D) > (*from.D)) ? FLAG_OF|FLAG_CF : 0;
		} else {
		(*to.W) -= (*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		State->Flags |= ((*to.W) > (*from.W)) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 51:
		DEBUG_S("XOR (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		(to) ^= (from);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		} else {
		(to) ^= (from);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		}
		break;

	case 59:
		DEBUG_S("CMP (RM)");
		ret = RME_Int_ParseModRM(State, &toB, &fromB);
		if(ret)	return ret;
		if(State->Decoder.bOverrideOperand){
		int v = (*to.D)-(*from.D);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((v) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((v) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(v) ^ PAIRITY16((v)>>16)) ? FLAG_PF : 0;
		State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;
		} else {
		int v = (*to.W)-(*from.W);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((v) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((v) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(v) ? FLAG_PF : 0;
		State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 4:
		READ_INSTR8( pt2 );
		DEBUG_S("ADD (AI) AL 0x%02x", pt2);
		(State->AX.B.L) += (pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.B.L) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.B.L) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(State->AX.B.L) ? FLAG_PF : 0;
		State->Flags |= (State->AX.B.L) < (pt2) ? FLAG_OF|FLAG_CF : 0
		break;

	case 12:
		READ_INSTR8( pt2 );
		DEBUG_S("OR (AI) AL 0x%02x", pt2);
		(State->AX.B.L) |= (pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.B.L) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.B.L) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(State->AX.B.L) ? FLAG_PF : 0;
		break;

	case 20:
		READ_INSTR8( pt2 );
		DEBUG_S("ADC (AI) AL 0x%02x", pt2);
		(State->AX.B.L) += (pt2) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.B.L) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.B.L) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(State->AX.B.L) ? FLAG_PF : 0;
		State->Flags |= ((State->AX.B.L) < (pt2)) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 28:
		READ_INSTR8( pt2 );
		DEBUG_S("SBB (AI) AL 0x%02x", pt2);
		(State->AX.B.L) -= (pt2) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.B.L) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.B.L) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(State->AX.B.L) ? FLAG_PF : 0;
		State->Flags |= ((State->AX.B.L) > (pt2)) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 36:
		READ_INSTR8( pt2 );
		DEBUG_S("AND (AI) AL 0x%02x", pt2);
		(State->AX.B.L) &= (pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.B.L) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.B.L) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(State->AX.B.L) ? FLAG_PF : 0;
		break;

	case 44:
		READ_INSTR8( pt2 );
		DEBUG_S("SUB (AI) AL 0x%02x", pt2);
		(State->AX.B.L) -= (pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.B.L) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.B.L) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(State->AX.B.L) ? FLAG_PF : 0;
		State->Flags |= ((State->AX.B.L) > (pt2)) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 52:
		READ_INSTR8( pt2 );
		DEBUG_S("XOR (AI) AL 0x%02x", pt2);
		(to) ^= (from);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.B.L) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.B.L) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(State->AX.B.L) ? FLAG_PF : 0;
		break;

	case 60:
		READ_INSTR8( pt2 );
		DEBUG_S("CMP (AI) AL 0x%02x", pt2);
		int v = (State->AX.B.L)-(pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((v) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((v) >> (7)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY8(v) ? FLAG_PF : 0;
		State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;
		break;

	case 5:
		if(State->Decoder.bOverrideOperand) {
		READ_INSTR32( dword );
		DEBUG_S("ADD (AIX) EAX 0x%08x", dowrd);
		(State->AX.D) += (dowrd);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(State->AX.D) ^ PAIRITY16((State->AX.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= (State->AX.D) < (dowrd) ? FLAG_OF|FLAG_CF : 0
		} else {
		READ_INSTR16( pt2 );
		DEBUG_S("ADD (AIX) AX 0x%04x", pt2);
		(State->AX.W) += (pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(State->AX.W) ? FLAG_PF : 0;
		State->Flags |= (State->AX.W) < (pt2) ? FLAG_OF|FLAG_CF : 0
		}
		break;

	case 13:
		if(State->Decoder.bOverrideOperand) {
		READ_INSTR32( dword );
		DEBUG_S("OR (AIX) EAX 0x%08x", dowrd);
		(State->AX.D) |= (dowrd);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(State->AX.D) ^ PAIRITY16((State->AX.D)>>16)) ? FLAG_PF : 0;
		} else {
		READ_INSTR16( pt2 );
		DEBUG_S("OR (AIX) AX 0x%04x", pt2);
		(State->AX.W) |= (pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(State->AX.W) ? FLAG_PF : 0;
		}
		break;

	case 21:
		if(State->Decoder.bOverrideOperand) {
		READ_INSTR32( dword );
		DEBUG_S("ADC (AIX) EAX 0x%08x", dowrd);
		(State->AX.D) += (dowrd) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(State->AX.D) ^ PAIRITY16((State->AX.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= ((State->AX.D) < (dowrd)) ? FLAG_OF|FLAG_CF : 0;
		} else {
		READ_INSTR16( pt2 );
		DEBUG_S("ADC (AIX) AX 0x%04x", pt2);
		(State->AX.W) += (pt2) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(State->AX.W) ? FLAG_PF : 0;
		State->Flags |= ((State->AX.W) < (pt2)) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 29:
		if(State->Decoder.bOverrideOperand) {
		READ_INSTR32( dword );
		DEBUG_S("SBB (AIX) EAX 0x%08x", dowrd);
		(State->AX.D) -= (dowrd) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(State->AX.D) ^ PAIRITY16((State->AX.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= ((State->AX.D) > (dowrd)) ? FLAG_OF|FLAG_CF : 0;
		} else {
		READ_INSTR16( pt2 );
		DEBUG_S("SBB (AIX) AX 0x%04x", pt2);
		(State->AX.W) -= (pt2) + ((State->Flags&FLAG_CF)?1:0);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(State->AX.W) ? FLAG_PF : 0;
		State->Flags |= ((State->AX.W) > (pt2)) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 37:
		if(State->Decoder.bOverrideOperand) {
		READ_INSTR32( dword );
		DEBUG_S("AND (AIX) EAX 0x%08x", dowrd);
		(State->AX.D) &= (dowrd);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(State->AX.D) ^ PAIRITY16((State->AX.D)>>16)) ? FLAG_PF : 0;
		} else {
		READ_INSTR16( pt2 );
		DEBUG_S("AND (AIX) AX 0x%04x", pt2);
		(State->AX.W) &= (pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(State->AX.W) ? FLAG_PF : 0;
		}
		break;

	case 45:
		if(State->Decoder.bOverrideOperand) {
		READ_INSTR32( dword );
		DEBUG_S("SUB (AIX) EAX 0x%08x", dowrd);
		(State->AX.D) -= (dowrd);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(State->AX.D) ^ PAIRITY16((State->AX.D)>>16)) ? FLAG_PF : 0;
		State->Flags |= ((State->AX.D) > (dowrd)) ? FLAG_OF|FLAG_CF : 0;
		} else {
		READ_INSTR16( pt2 );
		DEBUG_S("SUB (AIX) AX 0x%04x", pt2);
		(State->AX.W) -= (pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(State->AX.W) ? FLAG_PF : 0;
		State->Flags |= ((State->AX.W) > (pt2)) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 53:
		if(State->Decoder.bOverrideOperand) {
		READ_INSTR32( dword );
		DEBUG_S("XOR (AIX) EAX 0x%08x", dowrd);
		(to) ^= (from);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(State->AX.D) ^ PAIRITY16((State->AX.D)>>16)) ? FLAG_PF : 0;
		} else {
		READ_INSTR16( pt2 );
		DEBUG_S("XOR (AIX) AX 0x%04x", pt2);
		(to) ^= (from);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((State->AX.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((State->AX.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(State->AX.W) ? FLAG_PF : 0;
		}
		break;

	case 61:
		if(State->Decoder.bOverrideOperand) {
		READ_INSTR32( dword );
		DEBUG_S("CMP (AIX) EAX 0x%08x", dowrd);
		int v = (State->AX.D)-(dowrd);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((v) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((v) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(v) ^ PAIRITY16((v)>>16)) ? FLAG_PF : 0;
		State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;
		} else {
		READ_INSTR16( pt2 );
		DEBUG_S("CMP (AIX) AX 0x%04x", pt2);
		int v = (State->AX.W)-(pt2);
		State->Flags &= ~(FLAG_PF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_CF);
		State->Flags |= ((v) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((v) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(v) ? FLAG_PF : 0;
		State->Flags |= (v < 0) ? FLAG_OF|FLAG_CF : 0;
		}
		break;

	case 64:
		if(State->Decoder.bOverrideOperand) {
			DEBUG_S("INC EAX");
			to.D = &State->AX.D;
			State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
			(*to.D) ++;
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
		} else {
			DEBUG_S("INC AX");
			to.W = &State->AX.W;
			State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
			(*to.W) ++;
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
		}
		State->Flags |= (State->Flags & FLAG_ZF) ? FLAG_OF : 0;
		break;

	case 72:
		if(State->Decoder.bOverrideOperand) {
			DEBUG_S("DEC EAX");
			to.D = &State->AX.D;
			State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
			(*to.D) --;
		State->Flags |= ((*to.D) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.D) >> (31)) ? FLAG_SF : 0;
		State->Flags |= (PAIRITY16(*to.D) ^ PAIRITY16((*to.D)>>16)) ? FLAG_PF : 0;
			State->Flags |= (*to.D == 0xFFFFFFFF) ? FLAG_OF : 0;
		} else {
			DEBUG_S("DEC AX");
			to.W = &State->AX.W;
			State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
			(*to.W) --;
		State->Flags |= ((*to.W) == 0) ? FLAG_ZF : 0;
		State->Flags |= ((*to.W) >> (15)) ? FLAG_SF : 0;
		State->Flags |= PAIRITY16(*to.W) ? FLAG_PF : 0;
			State->Flags |= (*to.W == 0xFFFF) ? FLAG_OF : 0;
		}
		break;

	case 176:
		DEBUG_S("MOV (RI) AL");
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		State->AX.B.L = pt2;
		break;

	case 177:
		DEBUG_S("MOV (RI) CL");
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		State->CX.B.L = pt2;
		break;

	case 178:
		DEBUG_S("MOV (RI) DL");
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		State->DX.B.L = pt2;
		break;

	case 179:
		DEBUG_S("MOV (RI) BL");
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		State->BX.B.L = pt2;
		break;

	case 180:
		DEBUG_S("MOV (RI) AH");
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		State->SP.B.L = pt2;
		break;

	case 181:
		DEBUG_S("MOV (RI) CH");
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		State->BP.B.L = pt2;
		break;

	case 182:
		DEBUG_S("MOV (RI) DH");
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		State->SI.B.L = pt2;
		break;

	case 183:
		DEBUG_S("MOV (RI) BH");
		READ_INSTR8( pt2 );
		DEBUG_S(" 0x%02x", pt2);
		State->DI.B.L = pt2;
		break;

	case 184:
		if(State->Decoder.bOverrideOperand) {
		DEBUG_S("MOV (RIX) EAX");
		READ_INSTR32( dword );
		DEBUG_S(" 0x%08x", dword);
		State->AX.D = dword;
		} else {
		DEBUG_S("MOV (RIX) AX");
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		State->AX.W = pt2;
		}
		break;

	case 185:
		if(State->Decoder.bOverrideOperand) {
		DEBUG_S("MOV (RIX) ECX");
		READ_INSTR32( dword );
		DEBUG_S(" 0x%08x", dword);
		State->CX.D = dword;
		} else {
		DEBUG_S("MOV (RIX) CX");
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		State->CX.W = pt2;
		}
		break;

	case 186:
		if(State->Decoder.bOverrideOperand) {
		DEBUG_S("MOV (RIX) EDX");
		READ_INSTR32( dword );
		DEBUG_S(" 0x%08x", dword);
		State->DX.D = dword;
		} else {
		DEBUG_S("MOV (RIX) DX");
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		State->DX.W = pt2;
		}
		break;

	case 187:
		if(State->Decoder.bOverrideOperand) {
		DEBUG_S("MOV (RIX) EBX");
		READ_INSTR32( dword );
		DEBUG_S(" 0x%08x", dword);
		State->BX.D = dword;
		} else {
		DEBUG_S("MOV (RIX) BX");
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		State->BX.W = pt2;
		}
		break;

	case 188:
		if(State->Decoder.bOverrideOperand) {
		DEBUG_S("MOV (RIX) ESP");
		READ_INSTR32( dword );
		DEBUG_S(" 0x%08x", dword);
		State->SP.D = dword;
		} else {
		DEBUG_S("MOV (RIX) SP");
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		State->SP.W = pt2;
		}
		break;

	case 189:
		if(State->Decoder.bOverrideOperand) {
		DEBUG_S("MOV (RIX) EBP");
		READ_INSTR32( dword );
		DEBUG_S(" 0x%08x", dword);
		State->BP.D = dword;
		} else {
		DEBUG_S("MOV (RIX) BP");
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		State->BP.W = pt2;
		}
		break;

	case 190:
		if(State->Decoder.bOverrideOperand) {
		DEBUG_S("MOV (RIX) ESI");
		READ_INSTR32( dword );
		DEBUG_S(" 0x%08x", dword);
		State->SI.D = dword;
		} else {
		DEBUG_S("MOV (RIX) SI");
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		State->SI.W = pt2;
		}
		break;

	case 191:
		if(State->Decoder.bOverrideOperand) {
		DEBUG_S("MOV (RIX) EDI");
		READ_INSTR32( dword );
		DEBUG_S(" 0x%08x", dword);
		State->DI.D = dword;
		} else {
		DEBUG_S("MOV (RIX) DI");
		READ_INSTR16( pt2 );
		DEBUG_S(" 0x%04x", pt2);
		State->DI.W = pt2;
		}
		break;

	case 145:
	\if(State->Decoder.bOverrideOperand)	DEBUG_S("XCHG EAX");
		else	DEBUG_S("XCHG AX");
		if(State->Decoder.bOverrideOperand)
			XCHG(State->AX.D, State->CX.D);
		else
			XCHG(State->AX.W, State->CX.W);
		break;	case 146:
	\if(State->Decoder.bOverrideOperand)	DEBUG_S("XCHG EAX");
		else	DEBUG_S("XCHG AX");
		if(State->Decoder.bOverrideOperand)
			XCHG(State->AX.D, State->DX.D);
		else
			XCHG(State->AX.W, State->DX.W);
		break;	case 147:
	\if(State->Decoder.bOverrideOperand)	DEBUG_S("XCHG EAX");
		else	DEBUG_S("XCHG AX");
		if(State->Decoder.bOverrideOperand)
			XCHG(State->AX.D, State->BX.D);
		else
			XCHG(State->AX.W, State->BX.W);
		break;	case 148:
	\if(State->Decoder.bOverrideOperand)	DEBUG_S("XCHG EAX");
		else	DEBUG_S("XCHG AX");
		if(State->Decoder.bOverrideOperand)
			XCHG(State->AX.D, State->SP.D);
		else
			XCHG(State->AX.W, State->SP.W);
		break;	case 149:
	\if(State->Decoder.bOverrideOperand)	DEBUG_S("XCHG EAX");
		else	DEBUG_S("XCHG AX");
		if(State->Decoder.bOverrideOperand)
			XCHG(State->AX.D, State->BP.D);
		else
			XCHG(State->AX.W, State->BP.W);
		break;	case 150:
	\if(State->Decoder.bOverrideOperand)	DEBUG_S("XCHG EAX");
		else	DEBUG_S("XCHG AX");
		if(State->Decoder.bOverrideOperand)
			XCHG(State->AX.D, State->SI.D);
		else
			XCHG(State->AX.W, State->SI.W);
		break;	case 151:
	\if(State->Decoder.bOverrideOperand)	DEBUG_S("XCHG EAX");
		else	DEBUG_S("XCHG AX");
		if(State->Decoder.bOverrideOperand)
			XCHG(State->AX.D, State->DI.D);
		else
			XCHG(State->AX.W, State->DI.W);
		break;