 
//----------------------------------------------------------
typedef uint8_t pixelvalue;
#define P_SO(a,b) { if ((a)>(b)) P_SWAP((a),(b)); }
#define P_SWAP(a,b) { pixelvalue temp=(a);(a)=(b);(b)=temp; }
#define P_MA(a,b) { if ((a)>(b)) (b)=(a); }
#define P_MI(a,b) { if ((a)>(b)) (a)=(b); }

//------------------------------------------------------------
//packed char RGB image (uint32_t)
//does separate medians on R,G,B
//scrambles the input array!
static inline uint32_t median3(uint32_t *mm)
{
uint8_t *m=(uint8_t*)mm;
//     -R-              -G-              -B-
P_SO(m[0],m[4]); P_SO(m[1],m[5]); P_SO(m[2],m[6]);
P_MI(m[4],m[8]); P_MI(m[5],m[9]); P_MI(m[6],m[10]);
P_MA(m[0],m[4]); P_MA(m[1],m[5]); P_MA(m[2],m[6]);
return mm[1];
}

//------------------------------------------------------------
//packed char RGB image (uint32_t)
//does separate medians on R,G,B
//scrambles the input array!
static inline uint32_t median5(uint32_t *mm)
{
uint8_t *m=(uint8_t*)mm;
//     -R-              -G-              -B-
P_SO(m[0],m[4]);   P_SO(m[1],m[5]);   P_SO(m[2],m[6]);
P_SO(m[12],m[16]); P_SO(m[13],m[17]); P_SO(m[14],m[18]);
P_MI(m[4],m[16]);  P_MI(m[5],m[17]);  P_MI(m[6],m[18]);
P_MA(m[0],m[12]);  P_MA(m[1],m[13]);  P_MA(m[2],m[14]);
P_SO(m[4],m[8]);   P_SO(m[5],m[9]);   P_SO(m[6],m[10]);
P_MI(m[8],m[12]);  P_MI(m[9],m[13]);  P_MI(m[10],m[14]);
P_MA(m[4],m[8]);   P_MA(m[5],m[9]);   P_MA(m[6],m[10]);
return mm[2];
}

//------------------------------------------------------------
//packed char RGB image (uint32_t)
//does separate medians on R,G,B
//scrambles the input array!
static inline uint32_t median7(uint32_t *mm)
{
uint8_t *m=(uint8_t*)mm;
//     -R-              -G-              -B-
P_SO(m[0],m[20]);  P_SO(m[1],m[21]);  P_SO(m[2],m[22]);
P_SO(m[8],m[16]);  P_SO(m[9],m[17]);  P_SO(m[10],m[18]);
P_SO(m[0],m[12]);  P_SO(m[1],m[13]);  P_SO(m[2],m[14]);
P_SO(m[4],m[24]);  P_SO(m[5],m[25]);  P_SO(m[6],m[26]);
P_SO(m[12],m[20]); P_SO(m[13],m[21]); P_SO(m[14],m[22]);
P_MA(m[0],m[4]);   P_MA(m[1],m[5]);   P_MA(m[2],m[6]);
P_SO(m[8],m[24]);  P_SO(m[9],m[25]);  P_SO(m[10],m[26]);
P_MA(m[8],m[12]);  P_MA(m[9],m[13]);  P_MA(m[10],m[14]);
P_MI(m[16],m[20]); P_MI(m[17],m[21]); P_MI(m[18],m[22]);
P_MI(m[12],m[24]); P_MI(m[13],m[25]); P_MI(m[14],m[26]);
P_SO(m[4],m[16]);  P_SO(m[5],m[17]);  P_SO(m[6],m[18]);
P_MA(m[4],m[12]);  P_MA(m[5],m[13]);  P_MA(m[6],m[14]);
P_MI(m[12],m[16]); P_MI(m[13],m[17]); P_MI(m[14],m[18]);
return mm[3];
}

//------------------------------------------------------------
//packed char RGB image (uint32_t)
//does separate medians on R,G,B
//scrambles the input array!
static inline uint32_t median9(uint32_t *mm)
{
uint8_t *m=(uint8_t*)mm;
//     -R-              -G-              -B-
P_SO(m[4],m[8]);   P_SO(m[5],m[9]);   P_SO(m[6],m[10]);
P_SO(m[16],m[20]); P_SO(m[17],m[21]); P_SO(m[18],m[22]);
P_SO(m[28],m[32]); P_SO(m[29],m[33]); P_SO(m[30],m[34]);
P_SO(m[0],m[4]);   P_SO(m[1],m[5]);   P_SO(m[2],m[6]);
P_SO(m[12],m[16]); P_SO(m[13],m[17]); P_SO(m[14],m[18]);
P_SO(m[24],m[28]); P_SO(m[25],m[29]); P_SO(m[26],m[30]);
P_SO(m[4],m[8]);   P_SO(m[5],m[9]);   P_SO(m[6],m[10]);
P_SO(m[16],m[20]); P_SO(m[17],m[21]); P_SO(m[18],m[22]);
P_SO(m[28],m[32]); P_SO(m[29],m[33]); P_SO(m[30],m[34]);
P_MA(m[0],m[12]);  P_MA(m[1],m[13]);  P_MA(m[2],m[14]);
P_MI(m[20],m[32]); P_MI(m[21],m[33]); P_MI(m[22],m[34]);
P_SO(m[16],m[28]); P_SO(m[17],m[29]); P_SO(m[18],m[30]);
P_MA(m[12],m[24]); P_MA(m[13],m[25]); P_MA(m[14],m[26]);
P_MA(m[4],m[16]);  P_MA(m[5],m[17]);  P_MA(m[6],m[18]);
P_MI(m[8],m[20]);  P_MI(m[9],m[21]);  P_MI(m[10],m[22]);
P_MI(m[16],m[28]); P_MI(m[17],m[29]); P_MI(m[18],m[30]);
P_SO(m[16],m[8]);  P_SO(m[17],m[9]);  P_SO(m[18],m[10]);
P_MA(m[24],m[16]); P_MA(m[25],m[17]); P_MA(m[26],m[18]);
P_MI(m[16],m[8]);  P_MI(m[17],m[9]);  P_MI(m[18],m[10]);
return(mm[4]);
}

//------------------------------------------------------------
//packed char RGB image (uint32_t)
//does separate medians on R,G,B
//scrambles the input array!
static inline uint32_t median11(uint32_t *mm)
{
uint8_t *m=(uint8_t*)mm;
//     -R-              -G-              -B-
P_SO(m[12],m[28]); P_SO(m[13],m[29]); P_SO(m[14],m[30]);
P_SO(m[0],m[40]);  P_SO(m[1],m[41]);  P_SO(m[2],m[42]);
P_SO(m[28],m[40]); P_SO(m[29],m[41]); P_SO(m[30],m[42]);
P_SO(m[16],m[36]); P_SO(m[17],m[37]); P_SO(m[18],m[38]);
P_SO(m[0],m[12]);  P_SO(m[1],m[13]);  P_SO(m[2],m[14]);
P_SO(m[32],m[12]); P_SO(m[33],m[13]); P_SO(m[34],m[14]);
P_SO(m[4],m[24]);  P_SO(m[5],m[25]);  P_SO(m[6],m[26]);
P_SO(m[12],m[36]); P_SO(m[13],m[37]); P_SO(m[14],m[38]);
P_SO(m[20],m[24]); P_SO(m[21],m[25]); P_SO(m[22],m[26]);
P_MI(m[24],m[40]); P_MI(m[25],m[41]); P_MI(m[26],m[42]);
P_SO(m[8],m[24]);  P_SO(m[9],m[25]);  P_SO(m[10],m[26]);
P_SO(m[4],m[20]);  P_SO(m[5],m[21]);  P_SO(m[6],m[22]);
P_MA(m[0],m[4]);   P_MA(m[1],m[5]);   P_MA(m[2],m[6]);
P_SO(m[32],m[16]); P_SO(m[33],m[17]); P_SO(m[34],m[18]);
P_SO(m[16],m[4]);  P_SO(m[17],m[5]);  P_SO(m[18],m[6]);
P_MA(m[16],m[32]); P_MA(m[17],m[33]); P_MA(m[18],m[34]);
P_MI(m[24],m[4]);  P_MI(m[25],m[5]);  P_MI(m[26],m[6]);
P_MI(m[20],m[36]); P_MI(m[21],m[37]); P_MI(m[22],m[38]);
P_MA(m[8],m[32]);  P_MA(m[9],m[33]);  P_MA(m[10],m[34]);
P_SO(m[32],m[12]); P_SO(m[33],m[13]); P_SO(m[34],m[14]);
P_SO(m[28],m[20]); P_SO(m[29],m[21]); P_SO(m[30],m[22]);
P_MI(m[20],m[12]); P_MI(m[21],m[13]); P_MI(m[22],m[14]);
P_MA(m[28],m[32]); P_MA(m[29],m[33]); P_MA(m[30],m[34]);
P_SO(m[32],m[24]); P_SO(m[33],m[25]); P_SO(m[34],m[26]);
P_MA(m[32],m[20]); P_MA(m[33],m[21]); P_MA(m[34],m[22]);
P_MI(m[20],m[24]); P_MI(m[21],m[25]); P_MI(m[22],m[26]);
return mm[5];
}

//------------------------------------------------------------
//packed char RGB image (uint32_t)
//does separate medians on R,G,B
//scrambles the input array!
static inline uint32_t median13(uint32_t *mm)
{
uint8_t *m=(uint8_t*)mm;
//     -R-              -G-              -B-
    P_SO(m[40],m[12]); P_SO(m[41],m[13]); P_SO(m[42],m[14]);
    P_SO(m[24],m[40]); P_SO(m[25],m[41]); P_SO(m[26],m[42]);
    P_SO(m[44],m[4]);  P_SO(m[45],m[5]);  P_SO(m[46],m[6]);
    P_SO(m[20],m[16]); P_SO(m[21],m[17]); P_SO(m[22],m[18]);
    P_SO(m[0],m[32]);  P_SO(m[1],m[33]);  P_SO(m[2],m[34]);
    P_SO(m[4],m[12]);  P_SO(m[5],m[13]);  P_SO(m[6],m[14]);
    P_SO(m[20],m[0]);  P_SO(m[21],m[1]);  P_SO(m[22],m[2]);
    P_SO(m[28],m[4]);  P_SO(m[29],m[5]);  P_SO(m[30],m[6]);
    P_SO(m[32],m[40]); P_SO(m[33],m[41]); P_SO(m[34],m[42]);
    P_SO(m[32],m[48]); P_SO(m[33],m[49]); P_SO(m[34],m[50]);
    P_SO(m[16],m[48]); P_SO(m[17],m[49]); P_SO(m[18],m[50]);
    P_SO(m[12],m[48]); P_SO(m[13],m[49]); P_SO(m[14],m[50]);
    P_SO(m[28],m[44]); P_SO(m[29],m[45]); P_SO(m[30],m[46]);
    P_SO(m[36],m[8]);  P_SO(m[37],m[9]);  P_SO(m[38],m[10]);
    P_SO(m[0],m[8]);   P_SO(m[1],m[9]);   P_SO(m[2],m[10]);
    P_SO(m[16],m[4]);  P_SO(m[17],m[5]);  P_SO(m[18],m[6]);
    P_SO(m[44],m[0]);  P_SO(m[45],m[1]);  P_SO(m[46],m[2]);
    P_SO(m[16],m[36]); P_SO(m[17],m[37]); P_SO(m[18],m[38]);
    P_MA(m[28],m[20]); P_MA(m[29],m[21]); P_MA(m[30],m[22]);
    P_MI(m[8],m[4]);   P_MI(m[9],m[5]);   P_MI(m[10],m[6]);
    P_MA(m[16],m[24]); P_MA(m[17],m[25]); P_MA(m[18],m[26]);
    P_SO(m[20],m[36]); P_SO(m[21],m[37]); P_SO(m[22],m[38]);
    P_SO(m[36],m[0]);  P_SO(m[37],m[1]);  P_SO(m[38],m[2]);
    P_MI(m[12],m[0]);  P_MI(m[13],m[1]);  P_MI(m[14],m[2]);
    P_MA(m[20],m[24]); P_MA(m[21],m[25]); P_MA(m[22],m[26]);
    P_SO(m[8],m[12]);  P_SO(m[9],m[13]);  P_SO(m[10],m[14]);
    P_MA(m[44],m[24]); P_MA(m[45],m[25]); P_MA(m[46],m[26]);
    P_SO(m[36],m[8]);  P_SO(m[37],m[9]);  P_SO(m[38],m[10]);
    P_MA(m[32],m[36]); P_MA(m[33],m[37]); P_MA(m[34],m[38]);
    P_MI(m[40],m[8]);  P_MI(m[41],m[9]);  P_MI(m[42],m[10]);
    P_SO(m[36],m[40]); P_SO(m[37],m[41]); P_SO(m[38],m[42]);
    P_MA(m[36],m[24]); P_MA(m[37],m[25]); P_MA(m[38],m[26]);
    P_MI(m[40],m[12]); P_MI(m[41],m[13]); P_MI(m[42],m[14]);
    P_MI(m[24],m[40]); P_MI(m[25],m[41]); P_MI(m[26],m[42]);
return mm[6];
}

//------------------------------------------------------------
//packed char RGB image (uint32_t)
//does separate medians on R,G,B
//scrambles the input array!
static inline uint32_t median25(uint32_t *mm)
{
uint8_t *m=(uint8_t*)mm;
//     -R-              -G-              -B-
P_SO(m[0],m[4]);   P_SO(m[1],m[5]);   P_SO(m[2],m[6]);
P_SO(m[12],m[16]); P_SO(m[13],m[17]); P_SO(m[14],m[18]);
P_SO(m[8],m[16]);  P_SO(m[9],m[17]);  P_SO(m[10],m[18]);
P_SO(m[8],m[12]);  P_SO(m[9],m[13]);  P_SO(m[10],m[14]);
P_SO(m[24],m[28]); P_SO(m[25],m[29]); P_SO(m[26],m[30]);
P_SO(m[20],m[28]); P_SO(m[21],m[29]); P_SO(m[22],m[30]);
P_SO(m[20],m[24]); P_SO(m[21],m[25]); P_SO(m[22],m[26]);
P_SO(m[36],m[40]); P_SO(m[37],m[41]); P_SO(m[38],m[42]);
P_SO(m[32],m[40]); P_SO(m[33],m[41]); P_SO(m[34],m[42]);
P_SO(m[32],m[36]); P_SO(m[33],m[37]); P_SO(m[34],m[38]);
P_SO(m[48],m[52]); P_SO(m[49],m[53]); P_SO(m[50],m[54]);
P_SO(m[44],m[52]); P_SO(m[45],m[53]); P_SO(m[46],m[54]);
P_SO(m[44],m[48]); P_SO(m[45],m[49]); P_SO(m[46],m[50]);
P_SO(m[60],m[64]); P_SO(m[61],m[65]); P_SO(m[62],m[66]);
P_SO(m[56],m[64]); P_SO(m[57],m[65]); P_SO(m[58],m[66]);
P_SO(m[56],m[60]); P_SO(m[57],m[61]); P_SO(m[58],m[62]);
P_SO(m[72],m[76]); P_SO(m[73],m[77]); P_SO(m[74],m[78]);
P_SO(m[68],m[76]); P_SO(m[69],m[77]); P_SO(m[70],m[78]);
P_SO(m[68],m[72]); P_SO(m[69],m[73]); P_SO(m[70],m[74]);
P_SO(m[84],m[88]); P_SO(m[85],m[89]); P_SO(m[86],m[90]);
P_SO(m[80],m[88]); P_SO(m[81],m[89]); P_SO(m[82],m[90]);
P_SO(m[80],m[84]); P_SO(m[81],m[85]); P_SO(m[82],m[86]);
P_SO(m[92],m[96]); P_SO(m[93],m[97]); P_SO(m[94],m[98]);
P_SO(m[8],m[20]);  P_SO(m[9],m[21]);  P_SO(m[10],m[22]);
P_SO(m[12],m[24]); P_SO(m[13],m[25]); P_SO(m[14],m[26]);
P_SO(m[0],m[24]);  P_SO(m[1],m[25]);  P_SO(m[2],m[26]);
P_SO(m[0],m[12]);  P_SO(m[1],m[13]);  P_SO(m[2],m[14]);
P_SO(m[16],m[28]); P_SO(m[17],m[29]); P_SO(m[18],m[30]);
P_SO(m[4],m[28]);  P_SO(m[5],m[29]);  P_SO(m[6],m[30]);
P_SO(m[4],m[16]);  P_SO(m[5],m[17]);  P_SO(m[6],m[18]);
P_SO(m[44],m[56]); P_SO(m[45],m[57]); P_SO(m[46],m[58]);
P_SO(m[32],m[56]); P_SO(m[33],m[57]); P_SO(m[34],m[58]);
P_SO(m[32],m[44]); P_SO(m[33],m[45]); P_SO(m[34],m[46]);
P_SO(m[48],m[60]); P_SO(m[49],m[61]); P_SO(m[50],m[62]);
P_SO(m[36],m[60]); P_SO(m[37],m[61]); P_SO(m[38],m[62]);
P_SO(m[36],m[48]); P_SO(m[37],m[49]); P_SO(m[38],m[50]);
P_SO(m[52],m[64]); P_SO(m[53],m[65]); P_SO(m[54],m[66]);
P_SO(m[40],m[64]); P_SO(m[41],m[65]); P_SO(m[42],m[66]);
P_SO(m[40],m[52]); P_SO(m[41],m[53]); P_SO(m[42],m[54]);
P_SO(m[80],m[92]); P_SO(m[81],m[93]); P_SO(m[82],m[94]);
P_SO(m[68],m[92]); P_SO(m[69],m[93]); P_SO(m[70],m[94]);
P_SO(m[68],m[80]); P_SO(m[69],m[81]); P_SO(m[70],m[82]);
P_SO(m[84],m[96]); P_SO(m[85],m[97]); P_SO(m[86],m[98]);
P_SO(m[72],m[96]); P_SO(m[73],m[97]); P_SO(m[74],m[98]);
P_SO(m[72],m[84]); P_SO(m[73],m[85]); P_SO(m[74],m[86]);
P_SO(m[76],m[88]); P_SO(m[77],m[89]); P_SO(m[78],m[90]);
P_MA(m[32],m[68]); P_MA(m[33],m[69]); P_MA(m[34],m[70]);
P_SO(m[36],m[72]); P_SO(m[37],m[73]); P_SO(m[38],m[74]);
P_SO(m[0],m[72]);  P_SO(m[1],m[73]);  P_SO(m[2],m[74]);
P_MA(m[0],m[36]);  P_MA(m[1],m[37]);  P_MA(m[2],m[38]);
P_SO(m[40],m[76]); P_SO(m[41],m[77]); P_SO(m[42],m[78]);
P_SO(m[4],m[76]);  P_SO(m[5],m[77]);  P_SO(m[6],m[78]);
P_SO(m[4],m[40]);  P_SO(m[5],m[41]);  P_SO(m[6],m[42]);
P_SO(m[44],m[80]); P_SO(m[45],m[81]); P_SO(m[46],m[82]);
P_SO(m[8],m[80]);  P_SO(m[9],m[81]);  P_SO(m[10],m[82]);
P_MA(m[8],m[44]);  P_MA(m[9],m[45]);  P_MA(m[10],m[46]);
P_SO(m[48],m[84]); P_SO(m[49],m[85]); P_SO(m[50],m[86]);
P_SO(m[12],m[84]); P_SO(m[13],m[85]); P_SO(m[14],m[86]);
P_SO(m[12],m[48]); P_SO(m[13],m[49]); P_SO(m[14],m[50]);
P_SO(m[52],m[88]); P_SO(m[53],m[89]); P_SO(m[54],m[90]);
P_MI(m[16],m[88]); P_MI(m[17],m[89]); P_MI(m[18],m[90]);
P_SO(m[16],m[52]); P_SO(m[17],m[53]); P_SO(m[18],m[54]);
P_SO(m[56],m[92]); P_SO(m[57],m[93]); P_SO(m[58],m[94]);
P_SO(m[20],m[92]); P_SO(m[21],m[93]); P_SO(m[22],m[94]);
P_SO(m[20],m[56]); P_SO(m[21],m[57]); P_SO(m[22],m[58]);
P_SO(m[60],m[96]); P_SO(m[61],m[97]); P_SO(m[62],m[98]);
P_MI(m[24],m[96]); P_MI(m[25],m[97]); P_MI(m[26],m[98]);
P_SO(m[24],m[60]); P_SO(m[25],m[61]); P_SO(m[26],m[62]);
P_MI(m[28],m[64]); P_MI(m[29],m[65]); P_MI(m[30],m[66]);
P_MI(m[28],m[76]); P_MI(m[29],m[77]); P_MI(m[30],m[78]);
P_MI(m[52],m[84]); P_MI(m[53],m[85]); P_MI(m[54],m[86]);
P_MI(m[60],m[92]); P_MI(m[61],m[93]); P_MI(m[62],m[94]);
P_MI(m[28],m[52]); P_MI(m[29],m[53]); P_MI(m[30],m[54]);
P_MI(m[28],m[60]); P_MI(m[29],m[61]); P_MI(m[30],m[62]);
P_MA(m[4],m[36]);  P_MA(m[5],m[37]);  P_MA(m[6],m[38]);
P_MA(m[12],m[44]); P_MA(m[13],m[45]); P_MA(m[14],m[46]);
P_MA(m[20],m[68]); P_MA(m[21],m[69]); P_MA(m[22],m[70]);
P_MA(m[44],m[68]); P_MA(m[45],m[69]); P_MA(m[46],m[70]);
P_MA(m[36],m[68]); P_MA(m[37],m[69]); P_MA(m[38],m[70]);
P_SO(m[16],m[40]); P_SO(m[17],m[41]); P_SO(m[18],m[42]);
P_SO(m[24],m[48]); P_SO(m[25],m[49]); P_SO(m[26],m[50]);
P_SO(m[28],m[56]); P_SO(m[29],m[57]); P_SO(m[30],m[58]);
P_SO(m[16],m[24]); P_SO(m[17],m[25]); P_SO(m[18],m[26]);
P_MA(m[16],m[28]); P_MA(m[17],m[29]); P_MA(m[18],m[30]);
P_SO(m[48],m[56]); P_SO(m[49],m[57]); P_SO(m[50],m[58]);
P_MI(m[40],m[56]); P_MI(m[41],m[57]); P_MI(m[42],m[58]);
P_SO(m[24],m[28]); P_SO(m[25],m[29]); P_SO(m[26],m[30]);
P_SO(m[40],m[48]); P_SO(m[41],m[49]); P_SO(m[42],m[50]);
P_SO(m[24],m[40]); P_SO(m[25],m[41]); P_SO(m[26],m[42]);
P_MA(m[24],m[68]); P_MA(m[25],m[69]); P_MA(m[26],m[70]);
P_SO(m[48],m[68]); P_SO(m[49],m[69]); P_SO(m[50],m[70]);
P_MI(m[28],m[68]); P_MI(m[29],m[69]); P_MI(m[30],m[70]);
P_SO(m[28],m[40]); P_SO(m[29],m[41]); P_SO(m[30],m[42]);
P_SO(m[48],m[72]); P_SO(m[49],m[73]); P_SO(m[50],m[74]);
P_MA(m[28],m[48]); P_MA(m[29],m[49]); P_MA(m[30],m[50]);
P_MI(m[40],m[72]); P_MI(m[41],m[73]); P_MI(m[42],m[74]);
P_SO(m[48],m[80]); P_SO(m[49],m[81]); P_SO(m[50],m[82]);
P_MI(m[40],m[80]); P_MI(m[41],m[81]); P_MI(m[42],m[82]);
P_MA(m[40],m[48]); P_MA(m[41],m[49]); P_MA(m[42],m[50]);
return mm[12];
}

#undef P_SO
#undef P_SWAP
#undef P_MA
#undef P_MI


