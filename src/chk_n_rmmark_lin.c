
/*  This version is for use on ALPHA(UNIX)/LINUX  */
/*  But has notes on how to covert to  DOS or ALPHA/LINUX version & vice versa */
/*  now caters for extended header for upto 2 rf channels --desh 09-FEB-2002 */
/*  Also, does the splitting for 1-bit data from 2 channels
    in addition to that for 2-bit data of two channels */

/* HOWEVER, with the new EPLD, the packing of two-channel data
   for 1-bit mode is same as that for 2-bit mode ... hence the
   byte-split codes will also be same... This was realised
   only on 28th June 2002... and hence the modification was
   incorporated on that day.

     ---desh 28-June-2002

*/

/*  Noted that several times the markers are at the expected locations,
    but the marker values are wrong (offset by say, 8 or 16). This leading to
    reporting of slips when there was really no data missed. Hence, we now
    check for only the presence of a marker, instead of a marker value match.
    In any case, we now note the number of wrong markers, and report that count.
     ---desh 10-April-2007
*/


/*   chk_n_rmmark_lin.c   major modification from chk_n_rmmark_ws.c
     1) Now reads the header 
     2) Writes out now even the last incomplete data block.
     3) Provision to split files in the 2-channel (4-bit) mode
     4) Notes the number of markers found
 
	      --desh 18-JAN-2002

     Also, added some code to enable reading very-old headers
     as well as some modifications to write the output data
     properly (using open statement rather than fopen).

              --desh 01-May-2002

     It expects the full path for the input file and creates the
     output data file in the current directory with a _fixed tag.

     This program is to find out whether any data bytes are missing 
     because some timing mismatch in the FIFO read/write timing
     And to correct such slips & output the data without the markers

     Now you can output only a part of the data to the _fixed file
	   -- desh 25-Jun-97

     1.    whether at every " Nth " word increments by ONE
		" N "  user selectable
	  EPLD programmed is " new24.pof 24/6 : 3:16  " 
					  .pof : Chksum : 1500c7
     2. The starategy is as follows:
	----------------------------

	a.  Look for the mark +1 at the specified location;
	b.  IF not found look for " 00 " in the consecutive
	    locations then read the next Byte , which the NEXT
	    " mark " available, then continue the same way

       c.  NO assumption is made like there must exist a mark
	   within a Block; It may or may NOT be true     

  ------------------------------------------------------- */


/*  Important pts : short int  2 bytes, int : 4 bytes */

/* use the following include statements for ALPHA/LINUX instead of the set further below */

/* *** for files bigger than 2GB in size -yogesh *** */
//#define _FILE_OFFSET_BITS  64
/* *** =========== *** */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dirent.h"
#include <unistd.h>

/* use the following include statements for DOS instead of the above */
/*
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <sys/stat.h>
*/





int handle,clstat;          /* used for files */
/* int handle_o1; */
FILE * handle_o1; 
long int i;
long int j,kk,k,l,m,kk2,kk3;
unsigned int mark, n1, n2, last_mark, next_mark;
unsigned int possible_last_mark;
unsigned int last_lower_byte,upper_byte,lower_byte;
unsigned int itemp1,itemp2,itemp3, lower2a,lower2b,lower2c, upper2b,upper2c;
unsigned int lower3a,lower3b,lower3c,upper3b,upper3c;
unsigned int mark2a,mark2b,mark2c,mark3a,mark3b,mark3c;
int nargs;
unsigned int want_output,temp1,temp2,split_file,max_miss,temp_swap;
unsigned int byte_split[256];
long int file_size,file_cnt;
long long int input_size;
char datafile[80] = "obs.dat";

char in_path[80] = "test.gbd";
char extra_strg0[8] = "_fixed";
char extra_strg1[8] = "1_fixed";
char extra_strg2[8] = "2_fixed";
char extra_strg3[8] = "3_fixed";
char extra_strg4[8] = "4_fixed";
char extra_strg5[8] = "5_fixed";
char extra_strg6[8] = "6_fixed";

/* *** _yogesh *** */
    char file_name[128];
    long int blocks4k,res_bytes;
    long int *blpt,*lbpt;
/* *** =========== *** */



struct stat stbuf;
/* int stat(char *, struct stat *); */  /* uncomment this for DOS; else comment it out */





/*  ================  declarations of subroutines   =====================*/
    void file_open();
    void file_close();
    void read_n_write_frame();
/* *** _yogesh *** */
    void fsiz_4kblocks(char file_name[],long int **blpt,long int **lbpt);
/* *** =========== *** */
/*  ================  Start of the main routine   =====================*/
	

void main(int argc, char *argv[])
		      /* should be ...    void main() for DOS
			 and just         main() for ALPHA/LINUX */
    {
	nargs = 5;
	if(argc<nargs)
	  {printf("usage....\n");
	   printf("WORK/chk_n_rmmark_lin  <infile> <outfile> <split_file_code> <nslip_blocks_allowed>\n");
	   exit(0);
	  }
	else
	  {sscanf(argv[1],"%su",in_path);
	   sscanf(argv[2],"%su",datafile);
	   sscanf(argv[3],"%du",&split_file); 
	   sscanf(argv[4],"%du",&max_miss);
	  }
     file_open();
     /* *** _yogesh *** */
     blpt = &blocks4k;
     lbpt = &res_bytes;
     fsiz_4kblocks(in_path,&blpt,&lbpt);
     input_size=(long long) blocks4k*(long long)4096 + (long long )res_bytes ;
     printf(" file size : %lld\n ",input_size);
     /* *** =========== *** */
     read_n_write_frame();
     file_close();
    }
/*  ================  end of main routine  ==============================*/



void    file_open()
{




/* These 5 lines are for ALPHA/LINUX runs */
/*   printf(" input file path ?  ");
   scanf("%s",in_path);
   printf(" File name ?  (with out the path)   ");
   scanf("%s",datafile);
   strcat(in_path,datafile);
*/
/*       if((handle = open(in_path, O_CREAT | O_RDWR | O_BINARY, S_IWRITE)) == -1) */ /* for DOS */
/*       if((handle = open(in_path, O_RDONLY , S_IWRITE)) == -1)  */ /* for ALPHA(UNIX)/LINUX */

       if((handle = open(in_path, O_RDONLY , S_IWRITE)) == -1)   /* for ALPHA(UNIX)/LINUX */
	  {
	   perror("Error in opening the in-put file  :");
	  }
	 stat(in_path,&stbuf);
	 file_size = stbuf.st_size;
         input_size = (long long)file_size;
         printf(" file size (from stat-function) : %ld\n ",file_size);
//         printf(" file size : %lld\n ",input_size);
	 i =i+1;


         

}

void file_close()
	{
/*        int clstat; */

	  if((close(handle))==-1)
	    {printf("Error closing the input file");}
	  if(want_output)
/*	     {if((close(handle_o1))!=-1) */
	     {if((fclose(handle_o1))==0)
	       {printf(" Output now available in:    %s\n ",datafile);}
	      else
	       {printf("Error closing the output file");}
	     }
	}


void read_n_write_frame()
{  
/*   this function reads the DAS data file       */
/*   Check whteher after every N words, the count (lower 8 bits of a marker word) is incremented by 1 */
/************************************************************/

       int res,data_first;       /* 4 bytes; i.e. define as long in for DOS and as int for ALPHA/LINUX */
       unsigned short int a[8192],a2[8192],a3[8192],a_out[8193]; /* 2 bytes */
       unsigned int research,st_wrd = 0,ofset_wrd = 0,missed_cnt; 
       unsigned short int b[1024];
       unsigned short int noise[4096];
       long int no_of_8kbytes,left_bytes,left_words,Word_Count,percent;
       float per_ge,bytes_comp;
       long int this_word,wraps_so_far,missed_so_far,slip_size,last_word;
       long int possible_last_word;
       long int slip_size_in_bytes;
       long int blk_size,nblk_out,nmax_out,start_out,stop_out;
       int i_out, temp_byte[2], nbytes_got, missed_bytes_till_now;
       long int mark_locn[128],missed_samps[128],missed_byte[128],byte_count,n_mark;
       short int *out_pointer[2];
       long int out_due[2],out_due_set,out_size,actual_out,out_size_now;
       unsigned int has_header,very_old_header;
       long int total_markers,wrong_markers;
       char psrname[32];
       char data_file[32];
       char site_name[32] = "GBD";
       char observer[32] = "Unknown";
       float f_0,f_1,dm;
       int hr,mt,sec,dd,mm,yy;
       int invert,invert1,nbits,n_rfch,mark_gap; 
                 /* the above should be "long int" for DOS; else "int".. (ALPHA/LINUX). 
                    In any case, should be 4-bytes */
       double bw,bw1;

       double *dh;
       float *fh;
       int *ih; /* "long int" for DOS; else "int".. In any case, should be 4-bytes */
       char *ph, *ph1;
       short int *sih, dummy_sih;



/*       int  rand; */

       i_out = 0;
       actual_out = 0;
       nblk_out =0;
       nbytes_got = 0;
       nmax_out = 8192;    /* block size used for  output array is 8192 words */
       out_size = nmax_out/2; 
       very_old_header = 0;
       out_due[0] = (nmax_out/4)*3;
       out_due[1] = (nmax_out/4);
       out_due_set = 0;
       out_pointer[0] = (short int *)a_out;
       out_pointer[1] = out_pointer[0] + out_size;
       missed_so_far = 0;
       blk_size = 4096;    /* each block read is of 4096 words */

/*       printf(" does this file contain header ? type 1 if yes; else 0 \n");
       scanf("%d",&has_header);*/
       has_header=1;

/* *** _yogesh *** */
/*     If split_file_code >= 3, it will assume that file does not 
contain header; this provision made for Ooty data files which does 
not contain header */
       if(split_file > 2) has_header = 0;
/* *** ========== *** */

       if(has_header == 1)
//	   {file_size = file_size - 2048;}
           {input_size = input_size - 2048; }

/* *** _yogesh *** */

/*     if file is larger than 2 GBytes, ask for size */
/*     it was found that the code was taking a wrong value of
       of file size (in some cases where size > 2 GB), so now
       onwards, if nslips_allowed == 7213, it will ask for 
       manual entry of the file size */
      
/*****
       if(max_miss == 7213 )
       { printf("  \n");
         printf("MANUAL ENTRY REQUESTED (**FILE-SIZE LARGER THAN 2 GB?**)\n");
         printf("enter the no. of 8K-blocks in the data file(size/8192) :  ");
//         printf(" and the remaining no. of bytes in the data file :  ");
         scanf("%d",&no_of_8kbytes);
         left_bytes = 0;
         printf("  \n");
       }
       else
       {
         no_of_8kbytes = (file_size)/(2*blk_size);
         left_bytes    = (file_size - (no_of_8kbytes * 8192));
       }
      ***************/
 

       if(has_header == 1)
       {    no_of_8kbytes = (input_size)/(2*blk_size);
            left_bytes    = (input_size - (no_of_8kbytes * 8192));
       }
       else
       { no_of_8kbytes = blocks4k/2;
         left_bytes = (blocks4k - no_of_8kbytes*2)*4096 + res_bytes; 
       }

//     printf(" file_size (bytes): %ld \n",no_of_8kbytes*8192+left_bytes);
//     printf(" file_size (bytes): %lld \n",no_of_8kbytes*8192+left_bytes);
     printf(" file_size (bytes): %ld * 8192 + %ld\n",no_of_8kbytes,left_bytes);
     printf("no.of 8kb blocks  : %ld\n",no_of_8kbytes);
     printf("left_bytes        : %ld\n",left_bytes);

/* *** =========== *** */

       lseek(handle,0,SEEK_SET);

/* read the header bytes (2048) if has_header == 1 */
       if(has_header == 1)
	  {if((res = read(handle,b,2048)) != 2048)
				      /* reading  8k bytes at one time */
	    {
	     printf("\n\nErr. reading the obs.dat file.. hit any key to Quit\n");
/*             getchar();   */     /* To skip, look for any key typed from KBD */
	     printf(" no. of markers found : %ld\n",total_markers);
	     printf(" no. of wrong markers : %ld\n",wrong_markers);
	     printf(" no. of slip events : %d\n",missed_cnt);
	     exit(0);
	    }

	     printf(" Header block summary : \n");
	     printf(" ===================================\n");
	     /* interprete the header info */
	     dh = (double *) b ;
	     fh = (float *) b ;
	     ih = (int *) b ;       /* "long int" for DOS; else "int".. should be 4-bytes */
	     ph = (char *)b ;
             sih = (short int *) b ;
/*             printf("%lf %f %f %d %d %d %d %d %d %d\n",dh[0],fh[2],fh[3],ih[4],ih[5],ih[6],ih[7],ih[8],ih[9],ih[10]);  */
/*             printf("%f %f %f %f %d %d %d %d %d %d %d\n",fh[0],fh[1],fh[2],fh[3],ih[4],ih[5],ih[6],ih[7],ih[8],ih[9],ih[10]);  */
             printf("%f\n",dh[19]);
/* see if we have a very OLD header */
             yy = ih[7];
             if(yy>=96 && yy<100)yy = yy + 1900; 
             if(yy != 0)
               { if(yy<96)yy = yy + 2000; }
             /* if(yy>1996 && yy<2000) */ /* very OLD header */

             if(yy<2000)  /* very OLD header */
               {very_old_header = 1;
                site_name[0] = 'G';
                site_name[1] = 'B';
                site_name[2] = 'D';
                printf("THIS IS A VERY OLD HEADER \n");}
             else
               {very_old_header = 0;}

             if(very_old_header < 1)
               {memcpy(site_name, ph+108, 32);}


/*             printf("%lf %f %f %d %d %d %d %d %d %d\n",dh[0],fh[2],fh[3],ih[4],ih[5],ih[6],ih[7],ih[8],ih[9],ih[10]); */
/*           printf("%f %f %f %f %d %d %d %d %d %d %d\n",fh[0],fh[1],fh[2],fh[3],sih[4],sih[5],sih[6],sih[7],sih[8],sih[9],sih[10]);
*/
             if(very_old_header == 1)
               {bw = 1050.00;
                f_0 = 34.5;
               }
             else
               {bw = dh[0];
                f_0 = fh[2];
                if(f_0 < 1.)
                  {f_0 = 34.5;
                   fh[2] = f_0;
                   printf("THIS IS AN OLD HEADER... from GBD \n");
                   if(bw < 1.)
                     {bw = 1050.00;
                      dh[0] = bw;}
                   site_name[0] = 'G';
                   site_name[1] = 'B';
                   site_name[2] = 'D';
                   memcpy(ph+108,site_name,32);
                  }
                else
                  {if(site_name[0] != 'G')
                    {if(bw != dh[19])   /* just in case they are different */
                       {bw = dh[19];
                        dh[0] = bw;
                       }
                    }
                  }
/**** test  *********/
                if(bw < 0.0 )
 		{ bw = -bw;
                  dh[0] = bw; }
/**** test  *********/
                if(bw < 1.)
                  {
/**** test  *********/
             printf("*****************^^^^^^^^^*******\n");
             printf("%f\n",bw);
             printf("*****************^^^^^^^^^*******\n");
/**** test  *********/
		   bw = 1050.00;
                   dh[0] = bw;
                   printf("THIS IS AN OLD HEADER... from GBD \n");
                   if(f_0 < 1.)
                    {f_0 = 34.5;
                     fh[2] = f_0;}
                   site_name[0] = 'G';
                   site_name[1] = 'B';
                   site_name[2] = 'D';
                   memcpy(ph+108,site_name,32);
                  }
                else
                  {if(site_name[0] != 'G')
                    {
                     if(bw != dh[19])   /* just in case they are different */
                      {bw = dh[19];
                       dh[0] = bw;
                      }
                    }
                  }
               }
/* just make sure that the correct BW is used; if the first entry dh[0] is corrupted */
                   if(site_name[0] != 'G')
                    {bw1 = dh[19];
                      if(bw != bw1)   /* just in case they are different */
                      {bw = dh[19];
                       dh[0] = bw;
                      }
                     if(site_name[0] = 'M')
                       {if(site_name[1] = 'S')
                          {if(site_name[2] = 'T')
                             { bw = -bw;
                               dh[0] = bw;
                             }
                          }
                       }
                    }
             printf(" bandwidth of obs.n : %f kHz \n",bw);

             printf(" Centred at : %f MHz \n",f_0);

	     dm = fh[3];
	     printf(" DM : %f pc/cc \n",dm);

	     mark_gap = ih[4];
	     printf(" marker_interval found : %d words \n",mark_gap);
/*             if(mark_gap>8192)mark_gap = 4096;
             if(mark_gap<2048)mark_gap = 4096;
*/
             mark_gap = 4096;
	     printf(" marker_interval used : %d words \n",mark_gap);

             if(very_old_header == 1)
               {dd = sih[5];
                mm = sih[6];
                yy = sih[7];
               }
             else
               {dd = ih[5];
                mm = ih[6];
                yy = ih[7];
               }
	     printf(" date : %d-%d-%d \n",dd,mm,yy);
             if(very_old_header == 1)
               {hr = sih[8];
                mt = sih[9];
                sec = sih[10];
               }
             else
               {hr = ih[8];
                mt = ih[9];
                sec = ih[10];
               }


             if(yy >= 2007 )
             {  if(site_name[0] = 'M')
                       {if(site_name[1] = 'S')
                          {if(site_name[2] = 'T')
                             { bw = -bw;  /* from the observations in Jan.2007, 
                                             LO frequency at MST is lower than 
                                             sky frequency */
                               dh[0] = bw;
                             }
                          }
                       }
             }




	     printf(" IST at start : %d:%d:%d \n",hr,mt,sec);

	     memcpy(psrname, ph+44, 32);
	     printf(" Field_name %s \n",psrname);

	     memcpy(data_file, ph+76, 32);
	     printf(" original_data_file_name %s \n",data_file);

             
             if(very_old_header < 1)
               {memcpy(site_name, ph+108, 32);}

             printf(" Telescope site_code %s \n",site_name);

             ih[4] = mark_gap;
             ofset_wrd = mark_gap + 1;
/*             printf("very_old_header = %d\n",very_old_header); */
/*      do some restoring/updating job to take care of OLD-headers */
             if(very_old_header == 1)
               {
                dh[0] = 1050.;   //bw;
                fh[2] = 34.5;  //f_0;
                ih[5] = dd;
                ih[6] = mm;
                ih[7] = yy;
                ih[8] = hr;
                ih[9] = mt;
                ih[10] = sec;
                memcpy(ph+108,site_name,32);
               }
             else
               {
             printf("very_old_header is still = %d\n",very_old_header);
               }
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
		/* next usable: ih[48] or fh[48] or dh[24] or ph+192  */
	       }
	     else
	       {
		nbits = 2;
		n_rfch = 1;
		bw1 = bw;
		f_1 = f_0;
		printf("Observer Unknown (due to old header)\n");
		ih[35] = n_rfch; 
		ih[36] = nbits;
		fh[37] = f_1;
		dh[19] = bw1;
		memcpy(ph+160, observer, 32);   /* ih[40] */
	       }
	     printf(" %d channel data; %d-bits per sample\n",n_rfch,nbits);
	     if(n_rfch > 1)
	       {
		printf(" ===================================\n");
		printf(" bandwidth of 2nd channel : %f kHz \n",bw1);
		printf(" Centred at : %f MHz \n",f_1);
	       }
	     printf(" ===================================\n");
	   
	  }
       else        /* no header */
	  {
	   lseek(handle,0,SEEK_SET);   /* rewind to the beginning */
/* *** _yogesh *** */
           if(split_file > 2 && split_file < 7)
             { ofset_wrd = 8192;
               n_rfch = 4; }
           else
             { printf(" Enter the number of words between markers\n");
               scanf("%d",&ofset_wrd); }
               ofset_wrd = ofset_wrd + 1;

//           if(split_file > 2 && split_file < 7)
//           printf(" No. of channels ? \n");
//           scanf("%d",&n_rfch);
/* *** ========== *** */

	  }


/*     printf(" want output (_fixed) file ? 1 : if yes; 0 : else \n");
       scanf("%d",&want_output); */


       want_output=1;
       if(want_output == 1)

	 {if(n_rfch > 1)
	    { /* printf(" want the split_file (parts 1 or 2) ? Enter 1 or 2 : if yes; 0 : else \n");
	      scanf("%d",&split_file);  */
	      printf(" requested split_file_code is %d\n",split_file);
	    } 
	  else
	    { split_file = 0;
	    }
/*  next 6 line commented for DOS runs; uncomment these for ALPHA/LINUX runs */
          if(split_file == 0)
	      {strcat(datafile,extra_strg0);}
          if(split_file == 1)
	      {strcat(datafile,extra_strg1);}
          if(split_file == 2)
	      {strcat(datafile,extra_strg2);}

/* *** _yogesh *** */
    /* to extract last two bits from Ooty data  */ 
          if(split_file == 3)
              {strcat(datafile,extra_strg3);}
   /* following is to check other six bits from Ooty data.......*/
          if(split_file == 4)
              {strcat(datafile,extra_strg4);}
          if(split_file == 5)
              {strcat(datafile,extra_strg5);}
          if(split_file == 6)
              {strcat(datafile,extra_strg6);}
/* *** ========== *** */

	  printf(" Output File name will be :    %s ",datafile);
/*          if((handle_o1 = open(datafile, O_CREAT | O_RDWR | O_BINARY, S_IWRITE)) == -1) */ /* for DOS */
/*          if((handle_o1 = open(datafile, O_RDWR , S_IWRITE)) == -1)  */ /* for ALPHA(UNIX)/LINUX */
                           /*	  handle_o1 = fopen(datafile,"w"); */ /* open the output file */
            handle_o1 = fopen(datafile,"wb");  /* open the binary output file */
/*          if((handle_o1 = open(datafile, O_CREAT | O_RDWR , S_IRWXU)) == -1) */  /* for ALPHA(UNIX)/LINUX */
/*          {
           perror("Error in opening the out-put file  :");
           exit(0);
          } */
	  printf(" in the local directory !\n ");
	     if(has_header == 1)
	       {
		if(split_file > 1)   /* interchange the chnnels related info */
                   { fh[2] = f_1;
                     dh[0] = bw1;
                     fh[37] = f_0;
                     dh[19] = bw;

                     dm = fh[52];
                     per_ge = fh[53];
                     fh[52] = per_ge;
                     fh[53] = dm;

                     dm = fh[54];
                     per_ge = fh[55];
                     fh[54] = per_ge;
                     fh[55] = dm;

                     dm = fh[56];
                     per_ge = fh[57];
                     fh[56] = per_ge;
                     fh[57] = dm;

                     dm = fh[58];
                     per_ge = fh[59];
                     fh[59] = per_ge;
                     fh[57] = dm;

		   }
		fwrite(b,sizeof(short int),1024,handle_o1); 
/*                write(handle_o1,ih,2048); */
	       }
	 
	 }
       else
	 { printf("  NO output  File opened !\n ");
	 }


       if(want_output)
	 {/*printf(" enter start and end block no.s for output\n");
	  scanf("%ld %ld",&start_out,&stop_out); */
	  /* force to default values */
	  start_out = 1;
	  stop_out = no_of_8kbytes;   /* =no_of_8kbytes; for ALPHA runs, and could be some fixed no. =500; for DOS runs */


	  if(split_file > 0)
/* *** _yogesh *** */
	    {start_out = start_out/2 + 1;
	     stop_out = stop_out/2;
	    }  

	/*    { if((split_file/2)*2 == split_file )
		{ i = split_file; }
              else
	 	{ i = split_file + 1; }
	     start_out = start_out/i + 1;
             stop_out = stop_out/i;
            } */
/* *** ========== *** */


	  if(start_out<= 0)
	    {start_out = 1;}
	  if(stop_out> no_of_8kbytes)
	    {stop_out = no_of_8kbytes;}

	  i = stop_out - start_out + 1;
	  printf("%ld blocks from %ld will be output\n",i,start_out);
	  stop_out = stop_out - 1;           /* as our counting starts with 0 */
	  start_out = start_out - 1;
	 }


       printf("split file code -- %d \n",split_file );

       if(split_file > 0)
	   /* generate a look-up for splitting bytes depending
	      on the no. of bits per sample and channels to be
	      ouput 
	   */
         {
/*	 {if(nbits >1) */

	    {for (j = 0; j < 256; j = j+1)
	       {
		/* pick the ls-bits 1-2 & 5-6  if split_file eq. 1
		   or pick the ls-bits 3-4 & 7-8 if split_file eq. 2
		   and pack them compactly*/

		n1 = 1;
		if(split_file == 2 ) n1 = 4;
/* *** _yogesh *** */
                if(split_file == 3 ) n1 = 1; /* For Ooty data, 
                                             only ls-bits 1 & 2 are relevant */
		if(split_file == 4 ) n1 = 4;
          
                if(split_file == 5 ) n1 = 16;
     
                if(split_file == 6 ) n1 = 64; 
/* *** ========== *** */
                               

		k = j;
		temp1 = ((k/n1) & (0x03));
		byte_split[j] = temp1;
		n2 = n1 * 16;
		temp1 = ((k/n2) & (0x03));
                if(split_file <= 2) byte_split[j] += temp1*4;

/* *** _yogesh *** */

        /* to extract last two bits from Ooty data  */
                if(split_file == 3) 
                {  
                   byte_split[j] = byte_split[j] + 2;
                   if(byte_split[j] > 3) byte_split[j] = byte_split[j] - 4 ;
                }
        /* following was to check other 6-bits from Ooty data....*/
                if(split_file == 4) 
                { 
                   byte_split[j] += 2;
                   if(byte_split[j] > 3) byte_split[j] = byte_split[j] - 4 ;
                }
                if(split_file == 5)
                {
                   byte_split[j] += 2;
                   if(byte_split[j] > 3) byte_split[j] = byte_split[j] - 4 ;
                }
                if(split_file == 6)
                {
                   byte_split[j] += 2;
                   if(byte_split[j] > 3) byte_split[j] = byte_split[j] - 4 ;
                }   

/* *** ========= *** */

	       }                                        /* "j" loop ends */
	    }

/* *** _yogesh *** */
/**
       for (j = 0; j < 256; j = j+1)
       {
       printf("***********\n");
       printf("%d %04x\n ",j,byte_split[j]);
       printf("===========");
       }     **/
/* *** =========== *** */



/* Since the packing of 1-bit data is found to be same as 2-bit data in
   the new EPLD...  the above split-codes will also do for 1-bit data ...
   hence the following is commented out */

/*	  else   */     /* 1-bit data */
/*	    { printf("Generated codes for splitting 1-bit data\n");
	      for (j = 0; j < 256; j = j+1)
	       {
*/		/* pick the ls-bits 1,3,5 & 7  if split_file eq. 1
		   or pick the ls-bits 2,4,6 & 8 if split_file eq. 2
		   and pack them compactly*/

/*		n1 = 1;
		if(split_file>1)n1 = 2;

		k = j;
		temp1 = ((k/n1) & (0x01));
		byte_split[j] = temp1;
		
		n2 = n1 * 4;
		temp1 = ((k/n2) & (0x01));
		byte_split[j] += temp1*2;
		
		n2 = n1 * 16;
		temp1 = ((k/n2) & (0x01));
		byte_split[j] += temp1*4;
		
		n2 = n1 * 64;
		temp1 = ((k/n2) & (0x01));
		byte_split[j] += temp1*8;
*/	   /*     printf("%d %04x\n ",j,byte_split[j]); */

/*	       }               */                         /* "j" loop ends */
/*	    }                  */
	 }
/* now, we can proceed to read the data */
       wrong_markers = 0;
       total_markers = 0;
       byte_count = 0;
       missed_bytes_till_now = 0;
       i = 1;
       research = 0;    /* let us not search to begin with */
       l = 0;
       bytes_comp = 0;


/*       printf(" Enter the max. no. of missing blocks (>0) allowed at a time \n");
       printf("  2 would be a reasonable choice \n");
       scanf("%d",&max_miss);
*/
	 printf(" max allowed block misses is %d\n",max_miss);
/*       max_miss = 1;*/
       if(max_miss<=0)
	 {max_miss = 1;}

       nbytes_got = 0;
       missed_cnt = 0;
     /* To find out the first encountered COUNTER value */
       if(site_name[0] == 'G')      /* GBD data; one-channel mode machine*/
         {k = 0;                   /* IMPORTANT */
          mark = 0;}               /* 8 bit counter value */
                                   /* suggest the first encountered 
                                                       COUNTER value */
       else                        /* Two channels mode machine; PSN or 
                                                             UTR data */
         if(site_name[0] == 'P' || yy < 2006)
           {k =  - blk_size;         /* IMPORTANT for the first expected 
                                                        marker location */
            mark = 255;}             /* and the first psuedo marker */
         else
           {k = 0;                   /* IMPORTANT : the linux acq does not have
                                                    a psuedo-marker, hence treat
                                                    it like GBD acq*/
            mark = 0;}               /* 8 bit counter value */
                                     /* suggest the first encountered 
                                                        COUNTER value */


       while (i <= no_of_8kbytes)
	 {
          if(i==1)
          { 
	    if((res = read(handle,a,8192)) != 8192) 
				      /* reading  8k bytes at one time */
	    {
	     printf("\n\nErr. reading the obs.dat file.. hit any key to Quit\n");
/*             getchar();    */   /* To skip, look for any key typed from KBD */
	     printf(" no. of markers found : %ld\n",total_markers);
	     printf(" no. of wrong markers : %ld\n",wrong_markers);
	     printf(" no. of slip events : %d\n",missed_cnt);
             printf(" no. of one-byte misses : %ld\n",byte_count);
	     exit(0);
	    }
	    if((res = read(handle,a2,8192)) != 8192) 
				      /* reading  8k bytes at one time */
	    {
	     printf("\n\nErr. reading the obs.dat file.. hit any key to Quit\n");
	     printf(" no. of markers found : %ld\n",total_markers);
	     printf(" no. of wrong markers : %ld\n",wrong_markers);
	     printf(" no. of slip events : %d\n",missed_cnt);
             printf(" no. of one-byte misses : %ld\n",byte_count);
	     exit(0);
	    }
          }
          if(i <= no_of_8kbytes-2)
          {
	    if((res = read(handle,a3,8192)) != 8192) 
				      /* reading  8k bytes at one time */
	    {
	     printf("\n\nErr. reading the obs.dat file.. hit any key to Quit\n");
	     printf(" no. of markers found : %ld\n",total_markers);
	     printf(" no. of wrong markers : %ld\n",wrong_markers);
	     printf(" no. of slip events : %d\n",missed_cnt);
             printf(" no. of one-byte misses : %ld\n",byte_count);
	     exit(0);
	    }
          }
/* *** _yogesh *** */
       if(split_file > 2) goto skip_marker_check ;
/* *** =========== *** */

	  if(i==1)    /* Note the data in the first record for use 
                         in filling the missing samples */
	    {for(l=0;l<blk_size;l++)
		{noise[l] = a[l];}
	    }
	  n_mark = 0;
      possible_last_mark = 0;
/* define a psudo marker at sample no -1 */
	  mark_locn[n_mark] = -1; 
	  missed_samps[n_mark] = 0;
          last_lower_byte =  255; /* some non-zero value will do */
	  n_mark++;
	  l = 0;
adjust:   if(research == 1)
	    { /* printf("searching for mark : %d\n", mark); */
/* == = = = == ======================================*/
             printf ("========================================================\n");
             printf ("Searching for marker; i,previous_mark : %ld %d \n",i,mark);
             printf ("========================================================\n");
/* == = = = == ======================================*/
	     for (kk = l; kk <blk_size ; kk = kk+1)  /* Checking each 
                                                        time 8k bytes */
			     /* 4k words the incrementing values */
	     {                                 
	       j = a[kk]/256;    /* extract the upper byte */
               upper_byte = j;
               lower_byte = a[kk] - j*256;
               /*==============================================================*/
               if(upper_byte == 0 || last_lower_byte == 0)
                         // try confirming pairs/triplets, keep possibility of 1-byte slip
               { if(i <= no_of_8kbytes-2)
                 { kk2 = kk + ofset_wrd;
                   if(kk2>=blk_size)
                   { kk2 = kk2 - blk_size;
                     if(kk2 == 0)
                     { itemp1 = a[blk_size-1]; }
                     else
                     { itemp1 = a2[kk2-1]; }
                     itemp2 = a2[kk2];
                     if(kk2+1>=blk_size)
                     { itemp3 = a3[0]; }
                     else
                     { itemp3 = a2[kk2+1]; }
                   }
                   else
                   { 
                     itemp1 = a[kk2-1];
                     itemp2 = a[kk2];
                     if(kk2+1>=blk_size)
                     { itemp3 = a2[0]; }
                     else
                     { itemp3 = a[kk2+1]; }
                   }
                   lower2a = itemp1 - (itemp1/256)*256;
                   upper2b = itemp2/256;
                   lower2b = itemp2 - upper2b*256;
                   upper2c = itemp3/256;
                   lower2c = itemp3 - upper2c*256;
                   mark2a = 8199;  // some large number
                   mark2b = 8199;
                   mark2c = 8199;
                   if(lower2a == 0) { mark2a = upper2b; }
                   if(upper2b == 0) { mark2b = lower2b; }
                   if(lower2c == 0) { mark2c = upper2c; }

                   kk3 = kk + 2*ofset_wrd;
                   if(kk3>=2*blk_size)
                   { kk3 = kk3 - 2*blk_size;
                     if(kk3 == 0)
                     { itemp1 = a2[blk_size-1]; }
                     else
                     { itemp1 = a3[kk3-1]; }
                     itemp2 = a3[kk3];
                     if(kk3+1>=blk_size)
                     { itemp3 = 70000 ; } // > 65535
                     else
                     { itemp3 = a3[kk3+1]; }
                   }
                   else if (kk3>=blk_size)
                   { kk3 = kk3 - blk_size;
                     if(kk3 == 0)
                     { itemp1 = a[blk_size-1]; }
                     else
                     { itemp1 = a2[kk3-1]; }
                     itemp2 = a2[kk3];
                     if(kk3+1>=blk_size)
                     { itemp3 = a3[0]; }
                     else
                     { itemp3 = a2[kk3+1]; }
                   }
                   else
                   {
                     itemp1 = a[kk3-1]; 
                     itemp2 = a[kk3];
                     if(kk3+1>=blk_size)
                     { itemp3 = a2[0]; }
                     else
                     { itemp3 = a[kk3+1]; }
                   }
                   lower3a = itemp1 - (itemp1/256)*256;
                   upper3b = itemp2/256;
                   lower3b = itemp2 - upper3b*256;
                   if(itemp3 < 65536)
                   { upper3c = itemp3/256;
                     lower3c = itemp3 - upper3c*256; }
                   else
                   { upper3c = 8199;  // some large value
                     lower3c = 8199; }
                   mark3a = 8199;  // some large number
                   mark3b = 8199;
                   mark3c = 8199;
                   if(lower3a == 0) { mark3a = upper3b; }
                   if(upper3b == 0) { mark3b = lower3b; }
                   if(lower3c == 0) { mark3c = upper3c; }
  
                   if(upper_byte==0) {itemp3 = lower_byte;}
                   if(last_lower_byte==0) {itemp3 = upper_byte;}
                   itemp1 = 100;  // some large number
                   if((mark2a-itemp3)==1 || (mark2b-itemp3)==1 || (mark2c-itemp3)==1)
                   { itemp1 = 1;}
                   if((mark2a-itemp3)==-255 || (mark2b-itemp3)==-255 || (mark2c-itemp3)==-255)
                   { itemp1 = 1;}
                   itemp2 = 100;  // some large number
                   if((mark3a-itemp3)==2 || (mark3b-itemp3)==2 || (mark3c-itemp3)==2)
                   { itemp2 = 2;}
                   if((mark3a-itemp3)==-254 || (mark3b-itemp3)==-254 || (mark3c-itemp3)==-254)
                   { itemp2 = 2;}
                   if(itemp1==1 && itemp2==2)
                   { printf ("match (and TRIPLET confirmed) at block no, mark  : %ld %d \n",i,itemp3);
 	             this_word = (i-1)*blk_size + kk + 1 + missed_so_far;
		     wraps_so_far = this_word/(ofset_wrd*256);
                     m = itemp3-mark;
                     if(m<0) wraps_so_far++ ;
                     mark = itemp3;
		     //slip_size = this_word - wraps_so_far*ofset_wrd*256 - (mark+1)*ofset_wrd; 
		     slip_size = this_word - wraps_so_far*ofset_wrd*256 - (mark)*ofset_wrd; 
                     if(upper_byte==0)
                     { slip_size_in_bytes = slip_size*2; }
                     else  // that means last_lower_byte==0
                     { slip_size_in_bytes = slip_size*2 -1; }
                     /* a sanity check  */
		     if(slip_size>0)
		     { m = slip_size;
                       printf("+ve slip size: %ld  %ld",m,nmax_out/4);
		       if(m>(nmax_out/4))goto hold_on; }
                     goto pair_confirmed;
                   }
                   goto hold_on; 
                 }
                 /*==============================================================*/
	         else if((upper_byte==0 &&((lower_byte==last_mark) || (lower_byte==next_mark))) 
               || (last_lower_byte==0 && ((upper_byte==last_mark) || (upper_byte==next_mark))) ) 
                 {
				  /* TEST >=0)) */
				  /* looking for "00" in the upper byte */
				  /* and new mark >= 0 */
		    /* pre_March_2013  mark = a[kk]; */
                   if(upper_byte==0) //&&((lower_byte==last_mark) || (lower_byte==next_mark))) 
                   {
                     mark = lower_byte;
	             printf ("match(?) at block no, mark    : %ld %d \n",i,mark);
/*                  printf (" values around the mark -,+: %d %d \n",a[k-1],a[k+1]);  */
                     printf (" values around the mark -,+: %d %d \n",a[kk-1],a[kk+1]);
 	             this_word = (i-1)*blk_size + kk + 1 + missed_so_far;
		     wraps_so_far = this_word/(ofset_wrd*256); 
					/* as the mark is modulo 256 */
		     slip_size = this_word - wraps_so_far*ofset_wrd*256 - (mark+1)*ofset_wrd; 
                     slip_size_in_bytes = slip_size*2;
                   }
                   else 
                   {
                     mark = upper_byte;
		     printf ("match(?) at block no, byte_shifted mark    : %ld %d \n",i,mark);
                     printf (" values around the mark (irrelevant here) -,+: %d %d \n",a[k-1],a[k+1]);
                     this_word = (i-1)*blk_size + kk + 1 + missed_so_far;
	             wraps_so_far = this_word/(ofset_wrd*256); 
 		     slip_size = this_word - wraps_so_far*ofset_wrd*256 - (mark+1)*ofset_wrd; 
                     slip_size_in_bytes = slip_size*2 -1;
                   }

                   /* make some sanity checks for the reasonableness of this marker */
	           if(slip_size<0)
	           { m = -slip_size;
	  	   if(m>((int)(max_miss*ofset_wrd)))
                     { printf("Missed (%ld)more than expected !! (%d) \n",m,ofset_wrd);
                       printf("Slip size in bytes: %ld \n",slip_size_in_bytes);
                       goto hold_on; 
                     }
		   }
		   if(slip_size>0)
		   { m = slip_size;
		     if(m>(nmax_out/4))goto hold_on;
		   }
                   goto pair_confirmed;
                 }
                 /*----------------------------------------------------------*/
                 else
                 { goto hold_on; }
                 /*----------------------------------------------------------*/
pair_confirmed:	                                 
		 if(slip_size>0)
		 { printf ("\n Extra words at this slip : %ld \n",slip_size);
                 }
		 else
		 { m = -slip_size;
		   printf ("\n Missing words at this slip : %ld \n",m);
                 }
		 research = 0;
		 k = kk + 1;
		 mark_locn[n_mark] = kk;
		 missed_samps[n_mark] = -slip_size;
                 /*====================================================*/
                 missed_byte[n_mark] = slip_size*2 - slip_size_in_bytes;
                 if(missed_byte[n_mark] < 0)
                 { slip_size = slip_size + 1;
                   missed_byte[n_mark] = - missed_byte[n_mark]; }
                 if(missed_byte[n_mark] > 0) {byte_count++ ;}
                 /*====================================================*/
		 total_markers++;
		 n_mark++;
		 missed_so_far = missed_so_far - slip_size;
		     
		 goto next;
hold_on:         printf("!");     
               }
             last_lower_byte = lower_byte;
           }
	   printf("\n No match (mark,block): %d %ld\n", mark,i);
	   goto skip;
	  }
next:     j =((blk_size-k)/ofset_wrd);/*the no of markers ahead in this block */
	  if(j==0)
	    {
	     k =  k - blk_size;
	    }
	  if(j >= 1)               /* j markers expected in this block */
	    { 
	     research = 0;
	     m = 0;
	     while(research == 0 && m < j)
		  { m++;
/*                    if(site_name[0] == 'G') */   /* One channel mode machine,
 						      i.e. GBD */
/*                      {if(k<1)k = k + blk_size;}       */
		    k = k + ofset_wrd ;
		    if(k>blk_size)k = k-blk_size;
		    if(k>blk_size)k = k-blk_size;  /* just in case */
		    l = k -1;
		    mark = mark + 1;
		    if(mark==256) mark=0;
/*		    if (a[l] == mark) */  /* this was the old condition... gave false alarms leading to reserach, even when the data were not missing, but the marker value was wrong */ 
		    j = a[l]/256;    /* extract the upper byte */
		    if (j == 0)    /* this condition just checks for a presence of a marker-like word, not not any more for the exact value of the marker .... desh 10-April-2007 */
		      { /* printf ("\n mark %d in block %ld\n",mark,i); */    /* : No missing of the counter val. */
		        if (a[l] != mark)wrong_markers++;
			research = 0;
			mark_locn[n_mark] = l;
			missed_samps[n_mark] = 0;
                        missed_byte[n_mark] = 0;

                        if(site_name[0] != 'G')   /*  Two channels mode machine */
                          {if(mark == 1)
                             {if(a[l+1] == mark)
                              {k = k + 1;
                               l = l + 1;}
                             }
                          }

			last_mark = mark + 1;
			if(last_mark==256)last_mark = 0;
                        next_mark = last_mark + 1;
			if(next_mark==256)next_mark = 0;
			last_word = (i-1)*blk_size + l;
			total_markers++;
			n_mark++;
		      }
		    else
		      { research = 1;
			missed_cnt = missed_cnt + 1;
			printf ("\n SLIP DETECTED ! count: %d",missed_cnt);
			printf ("\n (e-mark,actual) at block : %d %d %ld\n", mark,a[l],i);
			l = k-ofset_wrd;
			if(l<=-1) l = 0;
			goto adjust;
		      }
		   }    /* " while research && ... loop is over " */
     
/*  The fact that we are here means we have found all the markers at the expected places */
	     k = k - blk_size;
	    }    /* if j>= 1 condition */
	  goto okay;
skip:     printf(" \n search to continue (s,i,mark,last_mark) : %d %ld %d %d\n",research,i,mark,last_mark);

okay:     mark_locn[n_mark] = blk_size;
			       /* define a psudo marker at sample no -1 */
	  missed_samps[n_mark] = 0;
          missed_byte[n_mark] = 0;
	  n_mark++;

	if(want_output)
	 {for(l=0;l<(n_mark-1);l++) /* deal with each region bounded by two markers */
	     {/* printf("block,n_marks,i_out,missed : %ld %ld %ld %ld\n",i,n_mark,i_out,missed_samps[l]); */  missed_bytes_till_now = missed_bytes_till_now + missed_byte[l];
              if(missed_bytes_till_now >= 2)
              { missed_samps[l] = missed_samps[l] + 1;
                missed_bytes_till_now = missed_bytes_till_now - 2;
              }
	      if(missed_samps[l]<0) 
		       /* allow overwritting for the extra samples */
	       { i_out = i_out + missed_samps[l];
		 if(i_out<0)i_out = i_out + nmax_out;
	       }
	      if(missed_samps[l]>0)  
			   /* we need to  fill in some missing samples */
		{kk = rand();
		 kk = kk - (kk/blk_size)*blk_size;
		 for(m=0;m<missed_samps[l];m++)
		    { kk = kk + 1; 
		      kk = kk - (kk/blk_size)*blk_size;
		      if(kk<0)kk=kk+blk_size; 
		      if(split_file > 0)
			{temp1 = noise[kk];
			 temp2 = temp1/256;
			 n1 = temp1 - temp2*256;
			 n2 = temp2;
			 if(nbytes_got < 1)
			   {temp1 = byte_split[n2]*16 + byte_split[n1];
			    a_out[i_out] = temp1;
			    nbytes_got = 1;}
			 else
			   {temp1 = (byte_split[n2]*16 + byte_split[n1])*256;
			    a_out[i_out] += temp1;
			    nbytes_got = 0;
			    i_out++;}
			}
		      else
			{a_out[i_out] = noise[kk];
			 i_out++;
			}

		      if(i_out==out_due[out_due_set])
			{ /* printf(" Output block %ld\n",nblk_out); */
			  if((nblk_out >= start_out) && (nblk_out<=stop_out))
			    {actual_out++;
/*                             write(handle_o1,out_pointer[out_due_set],(out_size*sizeof(short int)));} */
			     fwrite(out_pointer[out_due_set],sizeof(short int),out_size,handle_o1);} 
			  out_due_set++;
			  if(out_due_set>=2)out_due_set = 0;
			  nblk_out++;
			}
		      if(i_out>=nmax_out)i_out = i_out - nmax_out;
		    }
		} 

	      kk = mark_locn[l] + 1;
	      j = mark_locn[l+1];
	      if(j>kk)
		{ /* printf(" copying from index %d %d \n",kk,j); */
	       for(m=kk;m<j;m++)
		    {if(split_file > 0)
		       {temp1 = a[m];
			temp2 = temp1/256;
			n1 = temp1 - temp2*256;
			n2 = temp2;
			if(nbytes_got < 1)
			  {temp1 = byte_split[n2]*16 + byte_split[n1];
			   a_out[i_out] = temp1;
			   nbytes_got = 1;}
			else
			  {temp1 = (byte_split[n2]*16 + byte_split[n1])*256;
			   a_out[i_out] += temp1;
			   nbytes_got = 0;
		 /*        temp1 = a_out[i_out];
			   printf(" %04x ",temp1);  */
			   i_out++;}
		       }
		     else
		       {a_out[i_out] = a[m];
	     /*        temp1 = a_out[i_out];
		       printf(" %04x ",temp1);  */
			i_out++;
		       }

		      if(i_out==out_due[out_due_set])
			{ /* printf(" Output block %ld\n",nblk_out); */
			 if((nblk_out >= start_out) && (nblk_out<=stop_out))
			   {actual_out++;
/*                            write(handle_o1,out_pointer[out_due_set],(out_size*sizeof(short int)));} */
			    fwrite(out_pointer[out_due_set],sizeof(short int),out_size,handle_o1);} 
			  out_due_set++;
			  if(out_due_set>=2)out_due_set = 0;
			  nblk_out++;
			}
		      if(i_out>=nmax_out)i_out = i_out - nmax_out;
		    }
		}
	     }
	 }    
	 /*    printf(" input block %ld is used\n",i); */


/* *** _yogesh *** */
skip_marker_check: if(want_output)

        { if(split_file >= 3)

           { for(m=0;m<4096;m++)
                {  temp1 = a[m];
                   temp2 = temp1/256;
                   n1 = temp1 - temp2*256;
                   n2 = temp2;
                   /* following is for byte-swapping */
                   /* temp_swap = n1 ;
                    n1 = n2 ;
                    n2 = temp_swap ; */
                   /* ****************************** */                          
                   if(nbytes_got < 1)
                     {temp1 = byte_split[n2]*4 + byte_split[n1];
                      a_out[i_out] = temp1;
                      nbytes_got = 1;}  /* actual values of nbytes_got
                                           are half of written here */
                   else
	             {if(nbytes_got == 1)
	               {temp1 = (byte_split[n2]*4 + byte_split[n1])*16;
        	        a_out[i_out] += temp1;
                        nbytes_got = 2;}
		      else
                        {if(nbytes_got == 2)
                          {temp1 = (byte_split[n2]*4 + byte_split[n1])*256;
                           a_out[i_out] += temp1;
                           nbytes_got = 3;}
                         else 
			  {if(nbytes_got == 3)
	                    {temp1 = (byte_split[n2]*4 + byte_split[n1])*256*16;
        	             a_out[i_out] += temp1;
                 	     nbytes_got = 0;
	                     i_out++;}             
 		          }
			}
		     }    
                }
                { if((nblk_out >= start_out) && (nblk_out<=stop_out))
                     { actual_out++; }
                  fwrite(out_pointer[0],sizeof(short int),1024,handle_o1); 
		  i_out = 0;
                  out_due_set++;
                  if(out_due_set>=2)out_due_set = 0;
                  nblk_out++;
                }
                if(i_out>=nmax_out)i_out = i_out - nmax_out;        
           }
        }

/* *** ========== *** */

	i++;          /* one more block read  */
	for(l=0;l<blk_size;l++)
            {a[l] = a2[l];}
	for(l=0;l<blk_size;l++)
            {a2[l] = a3[l];}
      }      /* End of while loop */




      if(want_output)
	/*  see if any points are yet to be written out */
	{j = i_out - out_due_set*out_size;
	 if(j<0)
	   { /* printf(" Output block %ld\n",nblk_out); */
	    if((nblk_out >= start_out) && (nblk_out<=stop_out))
	      {actual_out++;
/*               write(handle_o1,out_pointer[out_due_set],(out_size*sizeof(short int)));} */
	       fwrite(out_pointer[out_due_set],sizeof(short int),out_size,handle_o1);} 
	     out_due_set++;
	     if(out_due_set>=2)out_due_set = 0;
	     nblk_out++;
	     j = i_out - out_due_set*out_size;
	   }
	 if(j>0)
	   { /* printf(" Output block %ld\n",nblk_out); */
	    if((nblk_out >= start_out) && (nblk_out<=stop_out))
	      {actual_out++;
/*               write(handle_o1,out_pointer[out_due_set],(j*sizeof(short int)));} */
	       fwrite(out_pointer[out_due_set],sizeof(short int),j,handle_o1);} 
	     out_due_set++;
	     if(out_due_set>=2)out_due_set = 0;
	     nblk_out++;
	   }
	}
      printf(" no. of slip events : %d\n",missed_cnt);
      printf(" no. of markers found : %ld\n",total_markers);
      printf(" no. of wrong markers : %ld\n",wrong_markers);
      printf(" no. of 4KWord blocks written out : %ld\n",actual_out);
      printf(" no. of one-byte misses : %ld\n",byte_count);
      return;
 }        /*  End of function read_n_write_frame */






/************************************************************************
  This function is a try to make counterpart of the FORTRAN subroutine
  "fsiz_4kblocks.f" which takes the file-name (with full path) as the 
  input and uses the command 'ls -ltr' to first find out its size(in Bytes)
  and then the number of 4K-blocks and left-bytes.
  For all C-language syntax and structures, fully guided by
  JAYANTH CHENNAMANGALAM.....almost like he wrote it.
                                  -- Yogesh Maan  04 March 2008
************************************************************************/

    void fsiz_4kblocks(file_name,blpt,lbpt)

    char file_name[128];
    long int  **blpt,**lbpt;
    {
       char sys_command[256]={0}, buff[256]={0}, size[25]={0};
       FILE *fp=NULL;
       int  i,j=0, space_count=0;
       long long int isize=0;
        char x[2] = {0};

     strcpy(sys_command,"ls -lgo ");
     strcat(sys_command,file_name);
     strcat(sys_command," > junk_flist.text");
     system(sys_command);

     fp = fopen("junk_flist.text","r");
     fread(buff,sizeof(buff),1,fp);
     fclose(fp);

     strcpy(sys_command,"rm -f junk_flist.text");
     system(sys_command);

     for (i=0; i<strlen(buff); i++)
     {
       if(buff[i]==' ')
       {
         space_count++;
         if(space_count==2)
         { break;}
       }
     }

     for (i++; buff[i]!=' '; i++)
     {
        size[j++] = buff[i];
     }
        for (i = strlen(size)-1; i >= 0;--i)
        {
                x[0] = size[i];
                isize += (long long) (atoi(x)*pow(10, strlen(size)-i-1));
//              printf("%d %lld\n", atoi(x), isize);
        }

     **blpt = (long int)(isize/(long long) 4096);
     **lbpt = (long int)(isize - (long long int)(**blpt)*(long long)4096);
     return;
     }

