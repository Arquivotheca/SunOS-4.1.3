/*
 *"@(#)instruction.c 1.1 7/30/92  Copyright Sun Microsystems";
 */

extern int halt_on_error;
extern unsigned long trap_flag;
extern unsigned long donot_dq;
extern unsigned long error_ok;
extern int verbose_mode;
struct	value {
	unsigned long floatsingle;
	unsigned long floatdouble;
};
struct value rnd[] = {
0x3f000000,   0x3fe00000,
0x3fc00000,   0x3ff80000,
0x40200000,   0x40040000,
0x40600000,   0x400c0000,
0x40900000,   0x40120000,
0x40b00000,   0x40160000,
0x40d00000,   0x401a0000,
0x40f00000,   0x401e0000,
0x41080000,   0x40210000,
0x41180000,   0x40230000,
0x41280000,   0x40250000,
0x41380000,   0x40270000,
0x41480000,   0x40290000,
0x41580000,   0x402a0000,
0x41680000,   0x402c0000,
0x41780000,   0x402f0000
};

struct value val[] = {
0	  ,   0,
0x3F800000,   0x3FF00000,
0x40000000,   0x40000000,
0x40400000,   0x40080000,	
0x40800000,   0x40100000,
0x40A00000,   0x40140000,
0x40C00000,   0x40180000,
0x40E00000,   0x401C0000,
0x41000000,   0x40200000,
0x41100000,   0x40220000,
0x41200000,   0x40240000,
0x41300000,   0x40260000,
0x41400000,   0x40280000,
0x41500000,   0x402A0000,
0x41600000,   0x402C0000,
0x41700000,   0x402E0000,
0x41800000,   0x40300000,
0x41880000,   0x40310000,
0x41900000,   0x40320000,
0x41980000,   0x40330000,
0x41a00000,   0x40340000,
0x41a80000,   0x40350000,
0x41b00000,   0x40360000,
0x41b80000,   0x40370000,
0x41c00000,   0x40380000,
0x41c80000,   0x40390000,
0x41d00000,   0x403a0000,
0x41d80000,   0x403b0000,
0x41e00000,   0x403c0000,
0x41e80000,   0x403d0000,
0x41f00000,   0x403e0000,
0x41f80000,   0x403f0000,
0x42000000,   0x40400000,
0x42040000,   0x40408000,
0x42080000,   0x40410000,
0x420c0000,   0x40418000,
0x42100000,   0x40420000,
0x42140000,   0x40428000,
0x42180000,   0x40430000,
0x421c0000,   0x40438000,
0x42200000,   0x40440000,
0x42240000,   0x40448000,
0x42280000,   0x40450000,
0x422c0000,   0x40458000,
0x42300000,   0x40460000,
0x42340000,   0x40468000,
0x42380000,   0x40470000,
0x423c0000,   0x40478000,
0x42400000,   0x40480000,
0x42440000,   0x40488000,
0x42480000,   0x40490000,
0x424c0000,   0x40498000,
0x42500000,   0x404a0000,
0x42540000,   0x404a8000,
0x42580000,   0x404b0000,
0x425c0000,   0x404b8000,
0x42600000,   0x404c0000,
0x42640000,   0x404c8000,
0x42680000,   0x404d0000,
0x426c0000,   0x404d8000,
0x42700000,   0x404e0000,
0x42740000,   0x404e8000,
0x42780000,   0x404f0000,
0x427c0000,   0x404f8000,
0x42800000,   0x40500000,
0x42820000,   0x40504000,
0x42840000,   0x40508000,
0x42860000,   0x4050c000,
0x42880000,   0x40510000,
0x428a0000,   0x40514000,
0x428c0000,   0x40518000,
0x428e0000,   0x4051c000,
0x42900000,   0x40520000,
0x42920000,   0x40524000,
0x42940000,   0x40528000,
0x42960000,   0x4052c000,
0x42980000,   0x40530000,
0x429a0000,   0x40534000,
0x429c0000,   0x40538000,
0x429e0000,   0x4053c000,
0x42a00000,   0x40540000,
0x42a20000,   0x40544000,
0x42a40000,   0x40548000,
0x42a60000,   0x4054c000,
0x42a80000,   0x40550000,
0x42aa0000,   0x40554000,
0x42ac0000,   0x40558000,
0x42ae0000,   0x4055c000,
0x42b00000,   0x40560000,
0x42b20000,   0x40564000,
0x42b40000,   0x40568000,
0x42b60000,   0x4056c000,
0x42b80000,   0x40570000,
0x42ba0000,   0x40574000,
0x42bc0000,   0x40578000,
0x42be0000,   0x4057c000,
0x42c00000,   0x40580000,
0x42c20000,   0x40584000,
0x42c40000,   0x40588000,
0x42c60000,   0x4058c000,
0x42c80000,   0x40590000,
0x42ca0000,   0x40594000,
0x42cc0000,   0x40598000,
0x42ce0000,   0x4059c000,
0x42d00000,   0x405a0000,
0x42d20000,   0x405a4000,
0x42d40000,   0x405a8000,
0x42d60000,   0x405ac000,
0x42d80000,   0x405b0000,
0x42da0000,   0x405b4000,
0x42dc0000,   0x405b8000,
0x42de0000,   0x405bc000,
0x42e00000,   0x405c0000,
0x42e20000,   0x405c4000,
0x42e40000,   0x405c8000,
0x42e60000,   0x405cc000,
0x42e80000,   0x405d0000,
0x42ea0000,   0x405d4000,
0x42ec0000,   0x405d8000,
0x42ee0000,   0x405dc000,
0x42f00000,   0x405e0000,
0x42f20000,   0x405e4000,
0x42f40000,   0x405e8000,
0x42f60000,   0x405ec000,
0x42f80000,   0x405f0000,
0x42fa0000,   0x405f4000,
0x42fc0000,   0x405f8000,
0x42fe0000,   0x405fc000,
0x43000000,   0x40600000,
0x43010000,   0x40602000,
0x43020000,   0x40604000,
0x43030000,   0x40606000,
0x43040000,   0x40608000,
0x43050000,   0x4060a000,
0x43060000,   0x4060c000,
0x43070000,   0x4060e000,
0x43080000,   0x40610000,
0x43090000,   0x40612000,
0x430a0000,   0x40614000,
0x430b0000,   0x40616000,
0x430c0000,   0x40618000,
0x430d0000,   0x4061a000,
0x430e0000,   0x4061c000,
0x430f0000,   0x4061e000,
0x43100000,   0x40620000,
0x43110000,   0x40622000,
0x43120000,   0x40624000,
0x43130000,   0x40626000,
0x43140000,   0x40628000,
0x43150000,   0x4062a000,
0x43160000,   0x4062c000,
0x43170000,   0x4062e000,
0x43180000,   0x40630000,
0x43190000,   0x40632000,
0x431a0000,   0x40634000,
0x431b0000,   0x40636000,
0x431c0000,   0x40638000,
0x431d0000,   0x4063a000,
0x431e0000,   0x4063c000,
0x431f0000,   0x4063e000,
0x43200000,   0x40640000,
0x43210000,   0x40642000,
0x43220000,   0x40644000,
0x43230000,   0x40646000,
0x43240000,   0x40648000,
0x43250000,   0x4064a000,
0x43260000,   0x4064c000,
0x43270000,   0x4064e000,
0x43280000,   0x40650000,
0x43290000,   0x40652000,
0x432a0000,   0x40654000,
0x432b0000,   0x40656000,
0x432c0000,   0x40658000,
0x432d0000,   0x4065a000,
0x432e0000,   0x4065c000,
0x432f0000,   0x4065e000,
0x43300000,   0x40660000,
0x43310000,   0x40662000,
0x43320000,   0x40664000,
0x43330000,   0x40666000,
0x43340000,   0x40668000,
0x43350000,   0x4066a000,
0x43360000,   0x4066c000,
0x43370000,   0x4066e000,
0x43380000,   0x40670000,
0x43390000,   0x40672000,
0x433a0000,   0x40674000,
0x433b0000,   0x40676000,
0x433c0000,   0x40678000,
0x433d0000,   0x4067a000,
0x433e0000,   0x4067c000,
0x433f0000,   0x4067e000,
0x43400000,   0x40680000,
0x43410000,   0x40682000,
0x43420000,   0x40684000,
0x43430000,   0x40686000,
0x43440000,   0x40688000,
0x43450000,   0x4068a000,
0x43460000,   0x4068c000,
0x43470000,   0x4068e000,
0x43480000,   0x40690000,
0x43490000,   0x40692000,
0x434a0000,   0x40694000,
0x434b0000,   0x40696000,
0x434c0000,   0x40698000,
0x434d0000,   0x4069a000,
0x434e0000,   0x4069c000,
0x434f0000,   0x4069e000,
0x43500000,   0x406a0000,
0x43510000,   0x406a2000,
0x43520000,   0x406a4000,
0x43530000,   0x406a6000,
0x43540000,   0x406a8000,
0x43550000,   0x406aa000,
0x43560000,   0x406ac000,
0x43570000,   0x406ae000,
0x43580000,   0x406b0000,
0x43590000,   0x406b2000,
0x435a0000,   0x406b4000,
0x435b0000,   0x406b6000,
0x435c0000,   0x406b8000,
0x435d0000,   0x406ba000,
0x435e0000,   0x406bc000,
0x435f0000,   0x406be000,
0x43600000,   0x406c0000,
0x43610000,   0x406c2000,
0x43620000,   0x406c4000,
0x43630000,   0x406c6000,
0x43640000,   0x406c8000,
0x43650000,   0x406ca000,
0x43660000,   0x406cc000,
0x43670000,   0x406ce000,
0x43680000,   0x406d0000,
0x43690000,   0x406d2000,
0x436a0000,   0x406d4000,
0x436b0000,   0x406d6000,
0x436c0000,   0x406d8000,
0x436d0000,   0x406da000,
0x436e0000,   0x406dc000,
0x436f0000,   0x406de000,
0x43700000,   0x406e0000,
0x43710000,   0x406e2000,
0x43720000,   0x406e4000,
0x43730000,   0x406e6000,
0x43740000,   0x406e8000,
0x43750000,   0x406ea000,
0x43760000,   0x406ec000,
0x43770000,   0x406ee000,
0x43780000,   0x406f0000,
0x43790000,   0x406f2000,
0x437a0000,   0x406f4000,
0x437b0000,   0x406f6000,
0x437c0000,   0x406f8000,
0x437d0000,   0x406fa000,
0x437e0000,   0x406fc000,
0x437f0000,   0x406fe000
};

unsigned long  neg_val[] = {
	0x80000000,
	0xBF800000,
	0xC0000000,
	0xC0400000, 
        0xC0800000, 
        0xC0A00000, 
        0xC0C00000, 
        0xC0E00000, 
        0xC1000000, 
        0xC1100000,  
        0xC1200000,  
        0xC1300000,  
        0xC1400000,  
        0xC1500000,  
        0xC1600000,  
        0xC1700000,  
        0xC1800000 
}; 
	

integer_to_float_sp(){
	int	i;
	unsigned long  result;

        clear_regs(0);
	for (i = 0; i < 200; i++) {
		result = int_float_s(i);
		if (result != val[i].floatsingle) {
			if (verbose_mode)
			printf("\nfitos failed: int = %d, expected / observed = %x / %x\n",
					i,val[i].floatsingle, result);
			return(-1);
		}
	}
	return(0);
}
integer_to_float_dp(){
        int     i;
        unsigned long  res1;

        clear_regs(0); 
        for (i = 0; i < 200; i++) {
                res1 = int_float_d(i);
		if (res1 != val[i].floatdouble) {
		  if (verbose_mode)
			printf("\nfitod failed: int = %d, expected / observed = %x / %x\n", 
                                        i,val[i].floatdouble, res1);
			return(-1);
		}
	}
	return(0);
}
float_to_integer_sp() {
	int     i;
        unsigned long  result;

	clear_regs(0);
        for (i = 0; i < 200; i++) { 
                result = float_int_s(val[i].floatsingle);
		if (result != i) {
			if (verbose_mode)
			printf("\nfstoi failed: int = %d, expected / observed = %x / %x\n",  
                                        i, i, result);
			return(-1);
		}
	}
	return(0);
}

/*DEAD CODE
 * single_int_round()
 * {
 * 	int     i;
 *         unsigned long  result;
 * 
 * 
 * 	for (i = 0; i < 16; i++) {
 * 		clear_regs(0);
 * 		result = sin_int_rnd(rnd[i].floatsingle);
 * 		if (result != i) {
 * 			if (verbose_mode)
 * 			printf("\nfintzs failed: int = %x.5, expected / observed = %x / %x\n",
 * 				i, rnd[i].floatsingle, result);
 *                         return(-1);
 * 		}
 * 	}
 * 	return(0);
 * }
 * double_int_round()
 * {
 * 	int     i;
 *         unsigned long  result;
 * 
 * 	for (i = 0; i < 16; i++) {
 * 
 * 		clear_regs(0);
 *         	result = doub_int_rnd(rnd[i].floatdouble);
 * 		if (result != i) { 
 * 			if (verbose_mode)
 *                 	printf("\nfintzd failed: int = %x.5, expected / observed = %x / %x\n",
 *                                 i, rnd[i].floatdouble, result); 
 *                        	return(-1); 
 * 		}
 * 	}
 *         return(0); 
 * } 
 */

	
float_to_integer_dp() {
        int     i;
        unsigned long  res1;

	clear_regs(0);
        for (i = 0; i < 200; i++) {
		res1 = float_int_d(val[i].floatdouble);
		if (res1 != i) {
		  if (verbose_mode)
			printf("\nfdtoi failed: int = %d, expected / observed = %x / %x\n",   
                                        i, i, res1);
			return(-1);
		}
	}
	return(0);
}

single_doub()
{
	int     i, j;
        unsigned long  result, pass;

	clear_regs(0);

	for (i = 0; i < 200; i++) {
		result = convert_sp_dp(val[i].floatsingle);
		if (result != val[i].floatdouble) {
		  if (verbose_mode)
			printf("\nfstod failed: int = %d, expected / observed = %x / %x\n",   
                                       i, val[i].floatdouble, result); 
                       	return(-1);
               	} 
	}

	return(0);
}
double_sing()
{
        int     i, j;
        unsigned long  result, pass;

	clear_regs(0);
        for (i = 0; i < 200; i++) {
		result = convert_dp_sp(val[i].floatdouble);
		if (result != val[i].floatsingle) {
		  if (verbose_mode)
			printf("\nfdtos failed: int = %d, expected / observed = %x / %x\n",    
                                       i, val[i].floatsingle, result); 
                       	return(-1); 
               	}  
	}
	return(0);
}

fmovs_ins()
{
	int     i;
        unsigned long  result;
	int	passcnt;	
	unsigned long data = 0x3F800000;
	
	clear_regs(0);
	result = move_regs(data);
	if (result != data) {
		if (verbose_mode)
		printf("\nfmovs failed : written %x to f0, read from f31 = %x\n",data, result);
		return(-1);
	}
	return(0);
}
get_negative_value_pn()
{
	int     i;
        unsigned long  result;

	clear_regs(0);

	for (i = 0; i < 17 ; i++) {
		result = negate_value(val[i].floatsingle);
		if (result != neg_val[i]) {

		   if (verbose_mode)
			printf("\nfnegs failed(from pos to neg): int = %d,  expected / observed =  %x / %x\n", 
					i, neg_val[i], result);
		   return(-1);
		}
	}
	return(0);
}
get_negative_value_np()
{
        int     i;
        unsigned long  result;
 
	clear_regs(0);
	for (i = 0; i < 17; i++) {
		result = negate_value(neg_val[i]);
		if (result != val[i].floatsingle) {
		   if (verbose_mode)
		    printf("\nfnegs failed (from neg. to pos): int = %d, expected / observed = %x / %x\n", 
				i, val[i].floatsingle, result);
		  return(-1);
		}
        }
	return(0);
}
fabs_ins()
{
	int  i, j;
	unsigned long result, pass ;

	clear_regs(0);
	for (i = 0; i < 17; i++) {
		result = absolute_value(neg_val[i]);
		if (result != val[i].floatsingle) {
		  	  if (verbose_mode)
			printf("\nfabs failed: int = %d, expected / observed = %x / %x\n", 
                                i, val[i].floatsingle, result);
                   	  return(-1);
                }
	}
	return(0);
}

addition_test_sp()
{
	int  i;
        unsigned long result;

	clear_regs(0);
        for (i = 0; i < 200; i++) {
		result = add_sp(val[i].floatsingle, val[1].floatsingle);
		if (result != (val[i+1].floatsingle)) {
		       if (verbose_mode)
			printf("\nfadds failed: int = %d, f0 = %x, f2 = %x, f0+f2 = f4 = %x\n", 
				i+1, val[i].floatsingle, val[1].floatsingle, result);
			return(-1);
		}
	}
	return(0);
}
addition_test_dp()
{
        int  i;  
        unsigned long result;
 
        clear_regs(0);
	for (i = 0; i < 200; i++) {
		result = add_dp(val[i].floatdouble, val[1].floatdouble);
		if (result != (val[i+1].floatdouble)) {
			if (verbose_mode)
			printf("\nfaddd failed: int = %d, f0 = %x, f2 = %x, f0+f2 = f4 =  %x\n", 
				i+1, val[i].floatdouble, val[1].floatdouble, result);
			return(-1);
		}
	}
	return(0);
}
subtraction_test_sp()
{
        int  i;
        unsigned long result;

	clear_regs(0);
        for (i = 1; i < 200; i++) {
		result = sub_sp(val[i].floatsingle, val[i-1].floatsingle);
		if (result != val[1].floatsingle) {
                	if (verbose_mode)
			printf("\nfsubs failed:int = %d, f0 = %x, f2 = %x, f0-f2 = f4 = %x\n", 
				i-1,val[i].floatsingle, val[i-1].floatsingle, result);
			return(-1);
		}
	}
	return(0);
}
subtraction_test_dp()
{
        int  i;
        unsigned long result;
 
        clear_regs(0); 
	for (i = 1; i < 200; i++) {
		result = sub_dp(val[i].floatdouble, val[i-1].floatdouble);
		if (result != val[1].floatdouble) {
			if (verbose_mode)
			printf("\bfsubd failed: int = %d, f0 = %x, f2 = %x, f0-f2 = f4 = %x\n", 
                                i-1,val[i].floatdouble, val[i-1].floatdouble, result);
			return(-1);
		}
	}
	return(0);
} 

division_test_sp()
{
	int     i;
        unsigned long result;

	clear_regs(0);
	for (i = 1; i < 200; i++) {
		result = div_sp(val[i].floatsingle, val[1].floatsingle);
		if (result != val[i].floatsingle) {
			if (verbose_mode) 
			printf("\nfdivs failed: int = %d, f0 = %x, f2 = %x, f0 / f2 = f4 = %x\n",
					i, val[i].floatsingle, val[1].floatsingle, result);
			return(-1);
		}

	}
	return(0);
}
division_test_dp()
{
        int     i;
        unsigned long result;
 
	clear_regs(0);
	for (i = 1; i < 200; i++) {
		result = div_dp(val[i].floatdouble, val[1].floatdouble);
		if (result != val[i].floatdouble) {
			if (verbose_mode)
			printf("\nfdivd failed: int = %d, f0 = %x, f2 = %x, f0 / f2 = f4 = %x\n", 
                                        i, val[i].floatdouble, val[1].floatdouble, result);
			return(-1);
		}
        }
	return(0);
}
multiplication_test_sp()
{
	int     i; 
        unsigned long result; 

	clear_regs(0);
        for (i = 0; i < 200; i++) { 
                result = mult_sp(val[i].floatsingle, val[1].floatsingle); 
		if (result != val[i].floatsingle) { 
                        if (verbose_mode)
			printf("\nfmuls failed: int = %d, f0 = %x, f2 = %x, f0 / f2 = f4 = %x\n", 
                                        i, val[i].floatsingle, val[1].floatsingle, result);
			return(-1);
		}
	}
	return(0);
}
multiplication_test_dp()
{
        int     i; 
        unsigned long result; 
 
	clear_regs(0);
	for (i = 0; i < 200; i++) {
                result = mult_dp(val[i].floatdouble, val[1].floatdouble); 
		if (result != val[i].floatdouble) { 
                        if (verbose_mode)
			printf("\nfmuld failed: int = %d, f0 = %x, f2 = %x, f0 / f2 = f4 = %x\n",  
                                        i, val[i].floatdouble, val[1].floatdouble, result); 
                       	return(-1); 
                } 

        }
	return(0); 
} 

squareroot_test_sp()
{
        int     i;
        unsigned long result, workvalue;
	
	if (fpu_is_weitek()) {
		printf("         ** Weitek fpu - fsqrt not available **\n");
		return(0); 		/* skip on Weitek fpu */
 	}

        clear_regs(0);
        for (i = 1; i < 200; i++) {
		workvalue = val[i].floatsingle;
                result = square_sp( mult_sp(workvalue,workvalue) ); 
                if (result != workvalue) {
                	printf("\nfsqrt failed: written / read = %x / %x\n",
				 workvalue, result);        
			return(1);
		}
	}
        return(0);		/* test passed - no errors detected */
}

squareroot_test_dp()
{
        int     i;
        unsigned long result, workvalue;

	if (fpu_is_weitek()) {
		printf("         ** Weitek fpu - fsqrt not available **\n");
		return(0); 		/* skip on Weitek fpu */
 	}

        clear_regs(0);
        for (i = 1; i < 200; i++) {
                 
		workvalue = val[i].floatdouble;
                result = square_dp( mult_dp(workvalue,workvalue) ); 
                if (result != workvalue) {
                	printf("\nfsqrt failed: written / read = %x / %x\n",
				 workvalue, result);        
			return(1);
		}
	}
        return(0);		/* test passed - no errors detected */
}

compare_sp()
{
        int     i;
        unsigned long  result;

        clear_regs(0);
        for (i = 0; i < 200; i++) {
                result = cmp_s(val[i].floatsingle, val[i].floatsingle);
                if ((result & 0xc00) != 0) {
                        if (verbose_mode)
			printf("\nfcmps failed: f0 = %d, f2 = %d : expected / observed = 0 / %x\n",
                                i,i,(result & 0xc00) >> 10);
                                return(-1);
                }
                result  = cmp_s(val[i].floatsingle, val[i+1].floatsingle);
                if ((result & 0xc00) != 0x400) {
                        if (verbose_mode)
			printf("\nfcmps failed: f0 = %d, f2 = %d : expected / observed = 1 /%x\n",
                                i,i+1, (result & 0xc00) >> 10); 
                                return(-1); 
                } 
                result  = cmp_s(val[i+1].floatsingle, val[i].floatsingle); 
                if ((result & 0xc00) != 0x800) { 
                        if (verbose_mode)
			printf("\nfcmps failed: f0 = %d, f2 = %d : expected / observed = 2 /%x\n",
                                i+1, i,(result & 0xc00) >> 10);  
                                return(-1);  
                }  
        
                result  = cmp_s(val[i].floatsingle,0x7f800400);
                if ((result & 0xc00) != 0xc00){  
                        if (verbose_mode)
			printf("\nfcmps failed: f0 = %d, f2 = NaN : expected / observed = 3 /%x\n", 
                                i,(result & 0xc00) >> 10);   
                                return(-1);  
                }
        }
	return(0);
}
compare_dp()
{
        int     i;
        unsigned long  result;
 
        clear_regs(0);
        for (i = 0; i < 200; i++) {
                result = cmp_d(val[i].floatdouble, val[i].floatdouble);
                if ((result & 0xc00) != 0) {
                        if (verbose_mode)
			printf("\nfcmpd failed: f0 = %d, f2 = %d : expected / observed = 0 / %x\n",
                                i,i,(result & 0xc00) >> 10); 
                      return(-1); 
                } 
                result = cmp_d(val[i].floatdouble, val[i+1].floatdouble); 
                if ((result & 0xc00) != 0x400) { 
                        if (verbose_mode)
			printf("\nfcmpd failed: f0 = %d, f2 = %d : expected / observed = 1 /%x\n", 
                                i, i+1,(result & 0xc00) >> 10);  
                        return(-1);  
                }  
                result = cmp_d(val[i+1].floatdouble, val[i].floatdouble); 
                if ((result & 0xc00) != 0x800) {  
                        if (verbose_mode)
			printf("\nfcmpd failed: f0 = %d, f2 = %d : expected / observed = 2 /%x\n", 
                                i+1, i,(result & 0xc00) >> 10);   
                       return(-1);  
                }   
                result = cmp_d(val[i].floatdouble, 0x7ff00080);
                if ((result & 0xc00) != 0xc00) {   
                        if (verbose_mode)
			printf("\nfcmpd failed: f0 = %d, f2 = NaN : expected / observed = 3 /%x\n",  
                                i, (result & 0xc00) >> 10);    
                       return(-1);   
                }
	}
	return(0);
}

branching()
{
	int	i;
	unsigned long	result;

	clear_regs(0);
        result = get_fsr();
        result = result & 0xF0700000; /* set all exception bits to zero */
        set_fsr(result);
        error_ok = 1;


	
	for (i = 0; i < 64; i++) {	
	
		if (result = branches(0, val[i].floatsingle, 0x7f800400)) {
			if (verbose_mode)
				printf("\nFBU failed.\n");
			return(-1);
		}
		if (result = branches(1, val[i+1].floatsingle, val[i].floatsingle)) {
			if (verbose_mode)
			printf("\nFBG failed: f0 = %x, f2 = %x.\n", i+1, i);
			return(-1);
                }
		if (result = branches(2, val[i].floatsingle, 0x7f800400)) {
                        if (verbose_mode)
			printf("\nFBUG (unordered) failed.\n");
                       	return(-1);
                }
                if (result = branches(2, val[i+1].floatsingle, val[i].floatsingle)) {
                        if (verbose_mode)
			printf("\nFBUG (greater) failed: f0 = %x, f2 = %x.\n", i+1, i);
                       	return(-1);   
                }
		if (result = branches(3, val[i].floatsingle,val[i+1].floatsingle)) {
                        if (verbose_mode)
			printf("\nFBL failed: f0 = %x, f2 = %x.\n", i, i+1);
			return(-1);
                }
                if (result = branches(4, val[i].floatsingle, 0x7f800400)) {
                        if (verbose_mode)
			printf("\nFBUL (unordered) failed.\n");
                        return(-1);
                }
		if (result = branches(4,val[i].floatsingle,val[i+1].floatsingle)) { 
                        if (verbose_mode)
			printf("\nFBUL (Less) failed: f0 = %x, f2 = %x.\n", i, i+1);
			return(-1);   
                } 
                if (result = branches(5, val[i].floatsingle,val[i+1].floatsingle)) {  
                        if (verbose_mode)
			printf("\nFBLG (Less) failed: f0 = %x, f2 = %x.\n", i, i+1); 
                        return(-1);    
                }  
                if (result = branches(5, val[i+1].floatsingle, val[i].floatsingle)) {
                        if (verbose_mode)
			printf("\nFBLG (Greater) failed: f0 = %x, f2 = %x.\n", i+1, i);
			return(-1);     
                }   
                if (result = branches(6, val[i].floatsingle,val[i+1].floatsingle)) {   
                        if (verbose_mode)
			printf("\nFBNE failed: f0 = %x, f2 = %x.\n", i, i+1); 
                        return(-1);    
                }  
		if (result = branches(7, val[i].floatsingle,val[i].floatsingle)) {    
                        if (verbose_mode)
			printf("\nFBE failed : f0 = %x, f2 = %x.\n", i, i);  
                       	return(-1);     
                }   
                if (result = branches(8, val[i].floatsingle, 0x7f800400)) { 
                        if (verbose_mode)
			printf("\nFBUE (unordered) failed.\n"); 
                        return(-1);   
                } 
                if (result = branches(8, val[i].floatsingle,val[i].floatsingle)) {     
                        if (verbose_mode)
			printf("\nFBUE (equal) failed : f0 = %x, f2 = %x.\n", i, i);   
                        return(-1);      
                }    
                if (result = branches(9, val[i].floatsingle,val[i].floatsingle)) {      
                        if (verbose_mode)
			printf("\nFBGE (equal) failed : f0 = %x, f2 = %x.\n", i, i);    
                        return(-1);       
                }     
                if (result = branches(9, val[i+1].floatsingle, val[i].floatsingle)) {
                        if (verbose_mode)
			printf("\nFBGE (greater) failed: f0 = %x, f2 = %x.\n", i+1, i);
                        return(-1);   
                }
                if (result = branches(10, val[i].floatsingle, 0x7f800400)) {  
                        if (verbose_mode)
			printf("\nFBUGE (unordered) failed.\n");  
                        return(-1);    
                }  
                if (result = branches(10, val[i].floatsingle,val[i].floatsingle)) {       
                        if (verbose_mode)
			printf("\nFBUGE (equal) failed : f0 = %x, f2 = %x.\n", i, i);     
                        return(-1);        
                }      
                if (result = branches(10, val[i+1].floatsingle, val[i].floatsingle)) { 
                        if (verbose_mode)
			printf("\nFBUGE (greater) failed: f0 = %x, f2 = %x.\n", i+1, i); 
                        return(-1);    
                } 
                if (result = branches(11, val[i].floatsingle,val[i+1].floatsingle)) {   
                        if (verbose_mode)
			printf("\nFBLE (Less) failed: f0 = %x, f2 = %x.\n", i, i+1);  
                        return(-1);     
                }   
                if (result = branches(11, val[i].floatsingle,val[i].floatsingle)) {        
                        if (verbose_mode)
			printf("\nFBLE (equal) failed : f0 = %x, f2 = %x.\n", i, i);      
                        return(-1);         
                }       
                if (result = branches(12, val[i].floatsingle, 0x7f800400)) {   
                        if (verbose_mode)
			printf("\nFBULE (unordered) failed.\n");   
                        return(-1);     
                }   
                if (result = branches(12, val[i].floatsingle,val[i+1].floatsingle)) {    
                        if (verbose_mode)
			printf("\nFBULE (Less) failed: f0 = %x, f2 = %x.\n", i, i+1);   
                        return(-1);      
                }    
                if (result = branches(12, val[i].floatsingle,val[i].floatsingle)) {         
                        if (verbose_mode)
			printf("\nFBULE (equal) failed : f0 = %x, f2 = %x.\n", i, i);       
                        return(-1);          
                }        
                if (result = branches(13, val[i].floatsingle,val[i+1].floatsingle)) {     
                        if (verbose_mode)
			printf("\nFBO failed: f0 = %x, f2 = %x.\n", i, i+1);    
                        return(-1);       
                }     
		if (result = branches(14, val[i].floatsingle,val[i+1].floatsingle)) {      
                        if (verbose_mode)
			printf("\nFBA failed: f0 = %x, f2 = %x.\n", i, i+1);     
                        return(-1);        
                }      
                if (result = branches(15, val[i].floatsingle,val[i+1].floatsingle)) {       
                        	if (verbose_mode)
			printf("\nFBN failed: f0 = %x, f2 = %x.\n", i, i+1);      
                        return(-1);         
                }       
        }    
	error_ok = 0;
	return(0);
}

no_branching()
{
        int     i;
        unsigned long   result;

	clear_regs(0);
        result = get_fsr();
        result = result & 0xF0700000; /* set all exception bits to zero */
        set_fsr(result);
        error_ok = 1;

	
         
	for (i = 0; i < 64; i++) {      
         
                if (!(result = branches(0, val[i].floatsingle, val[i].floatsingle))) {
                        if (verbose_mode)
			printf("\nFBU failed.\n");
                        return(-1);
                }
		if (!(result = branches(1,val[i].floatsingle, val[i].floatsingle))) {
                        if (verbose_mode) 
			printf("\nFBG failed: f0 = %x, f2 = %x.\n", i, i);
                        return(-1);
		}
		if (!(result = branches(2,val[i].floatsingle, val[i].floatsingle))) {  
                        if (verbose_mode)
			printf("\nFBUG  failed: f0 = %x, f2 = %x.\n", i, i);
                        return(-1);   
                }
		if (!(result = branches(3,val[i].floatsingle, val[i].floatsingle))) {  
                        if (verbose_mode)
			printf("\nFBLfailed: f0 = %x, f2 = %x.\n", i, i);
                        return(-1);
                }
		if (!(result = branches(4, val[i].floatsingle, val[i].floatsingle))) {  
                        if (verbose_mode)
			printf("\nFBUL failed: f0 = %x, f2 = %x.\n", i, i);
                        return(-1);
                }
		if (!(result = branches(5, val[i].floatsingle, val[i].floatsingle))) {    
                        if (verbose_mode)
			printf("\nFBLG failed: f0 = %x, f2 = %x.\n", i, i); 
                        return(-1);    
                } 

		if (!(result = branches(6, val[i].floatsingle, val[i].floatsingle))) {    
                        if (verbose_mode)
			printf("\nFBNE failed: f0 = %x, f2 = %x.\n", i, i); 
                        return(-1); 
                } 
                if (!(result = branches(7, val[i+1].floatsingle, val[i].floatsingle))) {     
                        if (verbose_mode)
			printf("\nFBE failed: f0 = %x, f2 = %x.\n", i+1 , i); 
                       	return(-1); 
                } 
                if (!(result = branches(8, val[i+1].floatsingle, val[i].floatsingle))) {
                        if (verbose_mode)
			printf("\nFBUE failed: f0 = %x, f2 = %x.\n", i+1 , i);
                        return(-1);
                }
                if (!(result = branches(9, val[i].floatsingle, val[i+1].floatsingle))) {
                        if (verbose_mode)
			printf("\nFBGEfailed: f0 = %x, f2 = %x.\n", i, i+1 );
                        return(-1);
                } 
                if (!(result = branches(10, val[i].floatsingle,val[i+1].floatsingle))) { 
                        if (verbose_mode)
			printf("\nFBUGEfailed: f0 = %x, f2 = %x.\n", i, i+1 ); 
                        return(-1); 
                }  
                if (!(result = branches(11, val[i+1].floatsingle, val[i].floatsingle))) {
                        if (verbose_mode)
			printf("\nFBLE failed: f0 = %x, f2 = %x.\n", i+1 , i);
                        return(-1);
                } 
                if (!(result = branches(12,  val[i+1].floatsingle, val[i].floatsingle))) { 
                        if (verbose_mode)
			printf("\nFBULE failed: f0 = %x, f2 = %x.\n", i+1 , i); 
                        return(-1); 
                }  
                if (!(result = branches(13, val[i].floatsingle, 0x7f800400))) {  
                        if (verbose_mode)
			printf("\nFBO failed.\n");
			return(-1);         
                }       
        }    
	error_ok = 0;
        return(0);
}

compare_sp_except()
{
	int     i;
        unsigned long  result;

	clear_regs(0);
	result = get_fsr(); 
        result = result | 0xF800000; 
        set_fsr(result); 
	error_ok = 1;

        for (i = 0; i < 200; i++) {
                result = cmp_s_ex(val[i].floatsingle, 0x7fbfffff);
		if (!trap_flag) {
                        if (verbose_mode)
		printf("\nfcmpxs failed: Exception did not occur. fsr = %x\n", result);
                        return(-1);
		}
	}
	error_ok = 0;
	return(0);
} 
compare_dp_except()
{
	int	i;
	unsigned long  result; 
 
        clear_regs(0); 
	result = get_fsr();  
        result = result | 0xF800000;  
        set_fsr(result);  
        error_ok = 1; 
 

        for (i = 0; i < 200; i++) { 
                result = cmp_d_ex(val[i].floatdouble, 0x7ff00080);
		if (!trap_flag) { 
                        if (verbose_mode)
		printf("\nfcmpxd failed: Exception did not occur. fsr = %x\n", result); 
                                return(-1); 
                } 

        } 
	error_ok = 0;
        return(0); 
} 
 

		

