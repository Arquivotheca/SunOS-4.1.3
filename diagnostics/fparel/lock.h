/*
 *      static char     fpasccsid[] = "@(#)lock.h 1.1 2/14/86 Copyright Sun Microsystems";
 *
 *      Copyright (c) 1985 by Sun Microsystems, Inc 
 *
 *
 */
struct registers
{
        u_long  reg_msw;
        u_long  reg_lsw;
};
struct shadow_regs
{
        u_long shreg_msw;
        u_long shreg_lsw;
};

struct registers user[] = {

        0xE0000C00,   0xE0000C04,
        0xE0000C08,   0xE0000C0C, 
        0xE0000C10,   0xE0000C14,
        0xE0000C18,   0xE0000C1C,
        0xE0000C20,   0xE0000C24,  
        0xE0000C28,   0xE0000C2C,   
        0xE0000C30,   0xE0000C34, 
        0xE0000C38,   0xE0000C3C,
        0xE0000C40,   0xE0000C44,
        0xE0000C48,   0xE0000C4C, 
        0xE0000C50,   0xE0000C54,
        0xE0000C58,   0xE0000C5C,
        0xE0000C60,   0xE0000C64,
        0xE0000C68,   0xE0000C6C,
        0xE0000C70,   0xE0000C74, 
        0xE0000C78,   0xE0000C7C,
        0xE0000C80,   0xE0000C84, 
        0xE0000C88,   0xE0000C8C, 
        0xE0000C90,   0xE0000C94, 
        0xE0000C98,   0xE0000C9C, 
        0xE0000CA0,   0xE0000CA4, 
        0xE0000CA8,   0xE0000CAC, 
        0xE0000CB0,   0xE0000CB4, 
        0xE0000CB8,   0xE0000CBC, 
        0xE0000CC0,   0xE0000CC4, 
        0xE0000CC8,   0xE0000CCC, 
        0xE0000CD0,   0xE0000CD4, 
        0xE0000CD8,   0xE0000CDC, 
        0xE0000CE0,   0xE0000CE4, 
        0xE0000CE8,   0xE0000CEC, 
        0xE0000CF0,   0xE0000CF4, 
        0xE0000CF8,   0xE0000CFC 
};

struct shadow_regs shadow[] = {
         
        0xE0000E00,   0xE0000E04,
        0xE0000E08,   0xE0000E0C,
        0xE0000E10,   0xE0000E14,
        0xE0000E18,   0xE0000E1C,
        0xE0000E20,   0xE0000E24,
        0xE0000E28,   0xE0000E2C,
        0xE0000E30,   0xE0000E34,
        0xE0000E38,   0xE0000E3C
};

struct dp_short
{
        u_long   addr;
};

struct dp_short dps[] = {

        0xE0000384,
        0xE000038C, 
        0xE0000394, 
        0xE000039C, 
        0xE00003A4, 
        0xE00003AC,    /* add for all the 16 registers */
        0xE00003B4,    /* reg <- reg op (operand) */
        0xE00003BC, 
        0xE00003C4, 
        0xE00003CC, 
        0xE00003D4,
        0xE00003DC, 
        0xE00003E4, 
        0xE00003EC, 
        0xE00003F4, 
        0xE00003FC 
};

struct dp_ext 
{
        u_long  addr;
        u_long  addr_lsw;
};

struct dp_ext  ext[] = {

        0xE0001304,   0xE0001800,
        0xE000130C,   0xE0001808, 
        0xE0001314,   0xE0001810, 
        0xE000131C,   0xE0001818, 
        0xE0001324,   0xE0001820, 
        0xE000132C,   0xE0001828, 
        0xE0001334,   0xE0001830, 
        0xE000133C,   0xE0001838, 
        0xE0001344,   0xE0001840, 
        0xE000134C,   0xE0001848, 
        0xE0001354,   0xE0001850, 
        0xE000135C,   0xE0001858, 
        0xE0001364,   0xE0001860, 
        0xE000136C,   0xE0001868, 
        0xE0001374,   0xE0001870, 
        0xE000137C,   0xE0001878 
};

struct dp_cmd
{
        u_long  data;
};

struct dp_cmd cmd[] = {

        0x0000,
        0x10041,
        0x20082,
        0x300C3,
        0x40104,
        0x50145,
        0x60186,
        0x701C7,
        0x80208,
        0x90249,
        0xA028A,
        0xB02CB,
        0xC030C,
        0xD034D,
        0xE038E,
        0xF03CF,
        0x100410,
        0x110451,
        0x120492,
        0x1304D3,
        0x140514,
        0x150555,
        0x160596,
        0x1705D7,
        0x180618,
        0x190659,
        0x1A069A,
        0x1B06DB,
        0x1C071C,
        0x1D075D,
        0x1E079E,
        0x1F07DF
};       
struct regs
{
        u_long  reg;
};

struct regs users[] = {

        0xE0000C00,
        0xE0000C08, 
        0xE0000C10,  
        0xE0000C18,  
        0xE0000C20,   
        0xE0000C28,    
        0xE0000C30,  
        0xE0000C38, 
        0xE0000C40,
        0xE0000C48,    
        0xE0000C50,   
        0xE0000C58,  
        0xE0000C60, 
        0xE0000C68,
        0xE0000C70,    
        0xE0000C78,   
        0xE0000C80,  
        0xE0000C88,    
        0xE0000C90,    
        0xE0000C98,    
        0xE0000CA0,    
        0xE0000CA8,    
        0xE0000CB0,    
        0xE0000CB8,    
        0xE0000CC0,    
        0xE0000CC8,    
        0xE0000CD0,    
        0xE0000CD8,    
        0xE0000CE0,    
        0xE0000CE8,    
        0xE0000CF0,    
        0xE0000CF8    
};
struct ptr_command ptr_cmd[] =
{
        0x10005,
        0x20046,
        0x30087,
        0x400C8,
        0x50109,
        0x6014A,
        0x7018B,
        0x801CC,
        0x9020D,
        0xA024E,
        0xB028F,
        0xC02D0,
        0xD0311,
        0xE0352,
        0xF0393,
        0x1003D4,
        0x110415,
        0x120456,
        0x130497,
        0x1404D8,
        0x150519,
        0x16055A,
        0x17059B,
        0x1805DC,
        0x19061D,
        0x1A065E,
        0x1B069F
};
u_long val[] = {

        0x3FF00000, /* for dp 1 */
        0x40000000, /* for dp 2 */
        0x40080000, /* for dp 3 */
        0x40100000, /* for dp 4 */
        0x40140000  /* for dp 5 */
};

struct pointers {
         
        u_long start_addr; /* starting address */
        u_long incr;       /* increment */
        u_long no_times;   /* number of times to do */
        u_long dp_addr;    /* starting address for dp */
        u_long dp_incr;    /* increment for dp address */
};
struct pointers shrt[] = 
{
        {       0xE0000380, 8, 16,  0xE0001000, 0 },
        {       0xE0000384, 8, 16,  0xE0001000, 0 },
        {       0x0,        0,  0,  0x0,        0 }
};
struct pointers extend[] = 
{
        {       0xE0001300, 8, 16, 0xE0001800, 8 },
        {       0xE0001304, 8, 16, 0xE0001800, 8 },
        {       0x0,        0,  0, 0x0,        0 }
};
u_long sp_short[] =
{
        0xE0000380,
        0xE0000388,
        0xE0000390,
        0xE0000398,
        0xE00003A0, 
        0xE00003A8, 
        0xE00003B0, 
        0xE00003B8, 
        0xE00003C0, 
        0xE00003C8, 
        0xE00003D0, 
        0xE00003D8, 
        0xE00003E0, 
        0xE00003E8, 
        0xE00003F0, 
        0xE00003F8 
};

