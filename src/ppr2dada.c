       
/* ppr2dada.c: Convert the data recorded by PPR (after the markers are
 *             checked for and removed) to 8-bits per sample, and write
 *             them in DADA format.
 *
 *             Byte_split part of the code adapted from das_spectro_lin.c.
 *
 *             --yogesh maan    Feb. 2022
 
 */ 
/*   
     Logic-codes for 2-bit data:
     bits
     d1   d0    -    Signal level    -   Value assigned - value needed
     -----------------------------------------------------------------
      0     0    :   More negative than         0             -3
                    - Vref
      0     1    :   Betn  - Vref & 0v          1             -1
      1     0    :   0 < Vin  < V ref           2             +1
      1     1    :   0 < Vin  > V ref           3             +3
  -------------------------------------------------------------------- */

//  Important pts : short int  2 bytes,  int : 4 bytes
/* *** for files bigger than 2GB in size -yogesh *** */
#define _FILE_OFFSET_BITS  64
/* *** =========== *** */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <float.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dirent.h"
#include <math.h>
#include <string.h>
#include <unistd.h>

FILE * handle_o1;
FILE * handle_o2;
int handle_i;                        // used for file
unsigned int i,j,k,n,l,m,o,temp;
long int file_size;
int nargs, byte_split[256][8], n_percent;
char datafile[64] = "obs.dat";
char out1_file[64] = "dedisp.dat";
char psrname[32], site_name[32], data_file[32];
int hr,mt,sec,dd,mm,yy,hund_sec;
struct stat stbuf;

/*  ================  declarations of subroutines   =====================*/
    void file_open();
    void file_close();
    void read_unpack_data();
/*  ================  Start of the main routine   =====================*/
void main(int argc, char *argv[])                                     
    {
        nargs = 3;
        if(argc<nargs)
          {printf("usage information: \n");
           printf("----------------------------------------------------------\n");
           printf("ppr2dada  <infile-name>  <outfile-name>  <percentage_data>\n");
           printf("<percentage_data> should be 100 for processing all of the available data.\n");
           printf("----------------------------------------------------------\n");
	   exit(0);
          }
        else
          {sscanf(argv[1],"%su",datafile);
           sscanf(argv[2],"%su",out1_file);
           sscanf(argv[3],"%du",&n_percent);
          }

     file_open();
     read_unpack_data();
     file_close();
     exit(0);
    }
/*  ================  end of main routine  ==============================*/

 
void  file_open()
    {
     if((handle_i = open(datafile, O_RDONLY , S_IWRITE)) == -1)
      {
       perror("Error in opening the file  : ");
       printf("%s",datafile);
       return;
      }
     stat(datafile,&stbuf);
     file_size = stbuf.st_size;

     i =i+1;
    }

void file_close()
    {
      int clstat;
      if((clstat = close(handle_i)) == -1)
      printf("Error closing the file %s",datafile);
    }


void read_unpack_data()
{  
/************************************************************/
    int res,left_bytes, b[1024];    // 4 bytes
    long int no_of_4kbytes, limit, nblocks_want;
    unsigned short int a[4096];  // Please do not change this

/*  presently maxchannels = 4096 */
    int d[4100*4];   /* the dimension should be 4*max_channels */
    char outdata[4100*4*2];
    char observer[32] = "Unknown";
    float  f_0,f_1,dm;
    int    invert,nbits,n_rfch, kk;
    unsigned int  mark_gap, rblock, has_header;
    double   t_samp_true, bw, f_clk,bw1;
    char out0_file[64] = "";

    double *dh;
    float *fh;
    int *ih;
    char *ph;
    FILE *infile;
    char dadaheader[4096];




    rblock = 4096;    // please do not change this
    
    printf(" file_size (bytes): %ld\n",file_size);
    i = 1;
    lseek(handle_i,0,SEEK_SET);
    if((res = read(handle_i,b,2048)) != 2048)
      {printf("\n\nErr. reading the input file.. %s .\n\n",datafile);
       exit(0);
       }
         
/* see if a header exists in the raw_data file; let us assume it is there to begin with */
      has_header = 1;
      printf(" Header block summary : \n");
      /* interprete the header info */
      fh = (float *) b ;
      dh = (double *) b ;
      ih = (int *) b ;
      ph = (char *)b ;
      printf("%f %f %f %d %d %d %d %d %d %d\n",dh[0],fh[2],fh[3],ih[4],ih[5],ih[6],ih[7],ih[8],ih[9],ih[10]);
      printf(" ===================================\n");
      mark_gap = ih[4];
      printf(" marker_interval : %d words \n",mark_gap);
      if(mark_gap != 4096)
        { printf(" The header has wrong entry for marker_interval. May be header is absent\n");
        }
      bw = dh[0];
      invert = 1;
      if(bw < 0.0)
        {bw = -bw;
         invert = -1;
        }
      f_clk = bw*(double)(2./1000.);    /* f_clk in MHz */
      printf(" bandwidth of obs.n : %f kHz \n",bw);
      f_0 = fh[2];  
      printf(" Centred at : %f MHz \n",f_0);
      dm = fh[3];
      printf(" header_DM : %f pc/cc \n",dm);
      printf(" marker_interval : %d words \n",mark_gap);

      dd = ih[5];
      mm = ih[6];
      yy = ih[7];
      printf(" date : %d-%d-%d \n",dd,mm,yy);
      hr = ih[8];
      mt = ih[9];
      sec = ih[10];
      printf(" IST at start : %d:%d:%d \n",hr,mt,sec);

      memcpy(psrname, ph+44, 32);
      printf(" Field_name %s \n",psrname);
      memcpy(data_file, ph+76, 32);
      printf(" original_data_file_name %s \n",data_file);
      memcpy(site_name, ph+108, 32);
      printf(" Telescope site_code %s \n",site_name);
      printf(" ===================================\n");
      /* header extention  begins here */
      n_rfch = ih[35];
      if(n_rfch > 0 && n_rfch < 3)
        {
         nbits = ih[36];
         f_1 = fh[37];
         bw1 = dh[19];
         memcpy(observer, ph+160, 32);   /* ih[40] */
         printf(" Observer %s \n",observer);
         printf(" ===================================\n");
         hund_sec = ih[48];
         /* next usable: ih[49] or fh[49] or dh[25-] or ph+196  */
        }
      else
        {
         nbits = 2;
         n_rfch = 1;
         bw1 = bw;
         f_1 = f_0;
         hund_sec = 0;
         printf("Observer Unknown (due to old header)\n");
        }
      printf(" %d channel data; %d-bits per sample\n",n_rfch,nbits);
      if(n_rfch > 1)
        {
         printf(" ===================================\n");
         printf(" bandwidth of 2nd channel : %f kHz \n",bw1);
         printf(" Centred at : %f MHz \n",f_1);
        }
      printf(" ===================================\n");

      no_of_4kbytes = (file_size-2048)/rblock;
      printf("no.of 4kbyte blocks    : %ld\n",no_of_4kbytes);
      left_bytes    = ((file_size) - (no_of_4kbytes * rblock));
      printf("left_bytes        : %d + a  2048-byte header read\n",left_bytes);

      t_samp_true = 1000.0/(2.0*bw);   // in micro-sec, Nyquist sampling time
      bw = ((double)(invert))*bw;

      printf(" Raw resolution : %g micro-sec\n",t_samp_true); 
      strcat(out0_file,out1_file);
      printf(" Opening the out-put dada file %s \n",out0_file);
      handle_o1 = fopen(out0_file,"w");  // open the output file
      //-------------------------------
      infile=fopen("header.txt","r");
      fread(dadaheader,sizeof(char),4096,infile);
      fclose(infile);
      fwrite(dadaheader,sizeof(char),4096,handle_o1);
      //-------------------------------


      nblocks_want = (no_of_4kbytes*n_percent)/100;
      printf(" %d percent of this available data (%ld blocks) will be processed \n",n_percent,nblocks_want);

      if(nblocks_want <= 0) { nblocks_want = no_of_4kbytes; }
      if(nblocks_want > no_of_4kbytes) { nblocks_want = no_of_4kbytes; }
// generate a trinary code based on the 4 two-bit 3-level samples in a byte
/* or codes for 1-bit 2-level system */
      for (j = 0; j < 256; j = j+1)
         {
           if(nbits > 1)
            { if(nbits == 8 || nbits == 6)
               {byte_split[j][0] = j;            // the value of j itself
                if(nbits == 6)
                  {if(j & (0x20))
                    { byte_split[j][0] = (j | (0xE0));}
                   else
                     { byte_split[j][0] = (j & (0x1F));}
                  }
               }
              else
               { n = 1;
                 for (o = 0; o <= 3; o = o+1)    //  deal with only one byte
                  {  k = j;
                     temp  = ((k/n) & (0x03));
                     byte_split[j][o] = 2*temp - 3;
                     n = n * 4;
                  }                                 // "o" loop ends
               }
            }
           else   /* one bit mode */
            { n = 1;
              for (o = 0; o <= 7; o = o+1)    //  deal with only one byte
               {  k = j;
                  temp  = ((k/n) & (0x01));
                  byte_split[j][o] = 2*temp - 1;
                  n = n * 2;
               }                                 // "o" loop ends

            }
         }                                        // "j" loop ends
      if(nbits == 6)nbits=8;  // for rest of the calculation treat it as 8bit numbers
/* now all the byte_split codes are ready */

      printf("No of blocks to be processed : %ld\n",nblocks_want);

    i = 1;
    while (i <= nblocks_want)
    {   
         if((res = read(handle_i,a,rblock)) != rblock) 
                             // reading rblock bytes, each time
         { printf("\n\nErr. reading the obs.dat file.. \n");
           printf("\n\nCurrent data-block (4k bytes) number: %d \n",i);
           exit(0); }

	 limit = rblock/sizeof(unsigned short int);
         m = 0;       // Initialize the Dest. index for d array
         for (l = 0; l < limit ; l++)  
         { 
              temp = a[l]/256;
              n = a[l] - temp*256;
              d[m] = byte_split[n][0];  // hopefully not reversed 
              m++;
              if(nbits < 8)
               {
                d[m] = byte_split[n][1];
                m++;
                d[m] = byte_split[n][2];
                m++;
                d[m] = byte_split[n][3];
                m++;
                if(nbits < 2)
                 {
                  d[m] = byte_split[n][4];
                  m++;
                  d[m] = byte_split[n][5];
                  m++;
                  d[m] = byte_split[n][6];
                  m++;
                  d[m] = byte_split[n][7];
                  m++;
                 }
               }

              n = temp;
              d[m] = byte_split[n][0];
              m++;
              if(nbits < 8)
               {
                d[m] = byte_split[n][1];
                m++;
                d[m] = byte_split[n][2];
                m++;
                d[m] = byte_split[n][3];
                m++;
                if(nbits < 2)
                 {
                  d[m] = byte_split[n][4];
                  m++;
                  d[m] = byte_split[n][5];
                  m++;
                  d[m] = byte_split[n][6];
                  m++;
                  d[m] = byte_split[n][7];
                  m++;
                 }
               }
         }
           
         for (kk=0; kk<m; kk++) {
	     outdata[kk]=(unsigned char) d[kk];}
	 fwrite(outdata,sizeof(char),m,handle_o1);
         m = i/10000;
         l = i - m*10000;
         if(l==0) {printf(" Pass No : %d\n", i);}
         i = i+1;                           // done to skip the loop
     }
     
     printf("\n Last Pass No : %d\n", i);
     fclose(handle_o1);
        
 }
