typedef  void * XYspace;
typedef struct t2_fontinfo {
	char    *data;
	char    *name;
	int     data_length;
	int     name_length;
} t2_fontinfo;

int     format_type2_fc(int *,void *,XYspace,short int,t2_fontinfo);
int format_fc(int *modeP,void *env, XYspace Sxy, short int format, long p, long v)
{
    t2_fontinfo *fip = (struct t2_fontinfo *)p;
    if (fip->data != 5 || fip->name != 6)
	abort();
}

int format_type2_fc(
   int    *modeP,                  
   void *env,                      
   XYspace  Sxy,                   
   short int format,               
   t2_fontinfo t2_fi)		   
{
        return(format_fc(modeP,env,Sxy,format,(long)(&t2_fi),(long)1));
}

int main()
{
   t2_fontinfo t2_fi;
   void *env;
   int mode;
   short format;
   XYspace Sxy;
   
   env = (void *)0;
   mode = 1;
   format = 3;
   Sxy = (void *)4;
   t2_fi.data = 5;
   t2_fi.name = 6;
   t2_fi.data_length = 7;
   t2_fi.name_length = 2;
   
   
   format_type2_fc (&mode, env, Sxy, format, t2_fi);
   
   return 0;
}

