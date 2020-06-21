#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
               unsigned char r,g,b;
              }Pixel;

void XORSHIFT32(unsigned int seed,unsigned int **R,unsigned int dimensiune_R)
{
    unsigned int x,i;

    (*R)[0]=seed;
    for(i=1;i<dimensiune_R;i++)
    {
        x=(*R)[i-1];

        x^=x<<13;
	    x^=x>>17;
	    x^=x<<5;

	    (*R)[i]=x;
    }

}

void incarcare_img(char *nume_imagine_bmp,Pixel **L,unsigned char **header)
{
    unsigned int w_img,h_img,padding;
    unsigned char RGB[3];
    int i,j;

    FILE *f=fopen(nume_imagine_bmp,"rb");

    if(f==NULL)
    {
        printf("Nu am putut deschide fisierul care contine imaginea pe care trebuie sa o liniarizez!\n");
        return;
    }

    *header=(unsigned char *)malloc(54);

    if(*header==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul care retine headerul imaginii!\n");
        return;
    }

    fread(*header,54,1,f);

    fseek(f,18,SEEK_SET);

    fread(&w_img,sizeof(int),1,f);
    fread(&h_img,sizeof(int),1,f);

    if(w_img%4!=0)
        padding=4-(3*w_img)%4;
    else
        padding=0;

    fseek(f,54,SEEK_SET);

    *L=(Pixel *)malloc(w_img*h_img*sizeof(Pixel));

    if(*L==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul care retine imaginea liniarizata!\n");
        return;
    }

    for(i=h_img-1;i>=0;i--)
    {
        for(j=0;j<w_img;j++)
        {
           fread(RGB,3,1,f);
           (*L)[w_img*i+j].r=RGB[2];
           (*L)[w_img*i+j].g=RGB[1];
           (*L)[w_img*i+j].b=RGB[0];
        }
        fseek(f,padding,SEEK_CUR);//sar peste padding
    }

    fclose(f);
}

void export_img(char *nume_imagine_export,Pixel *L,unsigned char *header)
{
    unsigned int w_img,h_img,padding;
    int i,j;

    FILE *f=fopen(nume_imagine_export,"w+b");

    if(f==NULL)
    {
        printf("Nu am putut deschide fisierul in care creez imaginea din vectorul liniarizat!\n");
        return;
    }

    fwrite(header,54,1,f);

    fseek(f,18,SEEK_SET);
    fread(&w_img,sizeof(int),1,f);
    fread(&h_img,sizeof(int),1,f);

    if(w_img%4!=0)
        padding=4-(3*w_img)%4;
    else
        padding=0;

    fseek(f,54,SEEK_SET);

    unsigned char completare[3]={0};

    for(i=h_img-1;i>=0;i--)
    {
        for(j=0;j<w_img;j++)
        {
           fwrite(&L[w_img*i+j].b,1,1,f);
           fwrite(&L[w_img*i+j].g,1,1,f);
           fwrite(&L[w_img*i+j].r,1,1,f);
        }
        if(padding)
            fwrite(completare,padding,1,f);
    }

    fclose(f);
}

void criptare(char *nume_img_init,char *nume_img_criptata,char *nume_fisier_cheie)
{
    FILE *fin,*fc;

    fin=fopen(nume_img_init,"rb");
    fc=fopen(nume_fisier_cheie,"r");

    if(fin==NULL)
    {
        printf("Nu am putut deschide fisierul care contine imaginea pe care trebuie sa o criptez!\n");
        return;
    }

    if(fc==NULL)
    {
        printf("Nu am putut deschide fisierul care contine cheia secreta!\n");
        return;
    }

    unsigned char *header;
    Pixel *P;

    incarcare_img(nume_img_init,&P,&header);//liniarizez imaginea in vectorul P

    unsigned int R0,SV,*R,*S,w_img,h_img,poz,aux;

    fscanf(fc,"%u%u",&R0,&SV);

    fclose(fc);

    fseek(fin,18,SEEK_SET);
    fread(&w_img,sizeof(int),1,fin);
    fread(&h_img,sizeof(int),1,fin);

    R=(unsigned int*)malloc(2*w_img*h_img*sizeof(int));

    if(R==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul de numere aleatoare!\n");
        return;
    }

    int i;

    XORSHIFT32(R0,&R,2*w_img*h_img);//generez cele 2*W*H-1 numere aleatoare in R

    S=(unsigned int*)malloc(w_img*h_img*sizeof(int));

    if(S==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul care contine permutarea!\n");
        return;
    }

    for(i=0;i<w_img*h_img;i++)//generez permutarea identica in vectorul S
        S[i]=i;

    for(i=w_img*h_img-1;i>=1;i--)//generez permutarea aleatoare folosind numerele din R
    {
        poz=R[w_img*h_img-i]%(i+1);
        aux=S[poz];
        S[poz]=S[i];
        S[i]=aux;
    }

    Pixel *P1,*C;

    P1=(Pixel *)malloc(w_img*h_img*sizeof(Pixel));

    if(P1==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul care retine imaginea permutata!\n");
        return;
    }

    for(i=0;i<w_img*h_img;i++)//creez imaginea permutata P1 folosindu-ma de permutarea S
        memcpy(P1+S[i],P+i,sizeof(Pixel));

    C=(Pixel *)malloc(w_img*h_img*sizeof(Pixel));

    if(C==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul care retine imaginea criptata!\n");
        return;
    }

    //creez vectorul C care contine imaginea criptata

    unsigned char *z,*q;

    z=(unsigned char*)&SV;
    q=(unsigned char*)&R[w_img*h_img];

    C[0].b=z[0]^P1[0].b^q[0];
    C[0].g=z[1]^P1[0].g^q[1];
    C[0].r=z[2]^P1[0].r^q[2];

    for(i=1;i<w_img*h_img;i++)
        {
            z=(unsigned char*)&R[w_img*h_img+i];
            C[i].b=C[i-1].b^P1[i].b^z[0];
            C[i].g=C[i-1].g^P1[i].g^z[1];
            C[i].r=C[i-1].r^P1[i].r^z[2];
        }

    export_img(nume_img_criptata,C,header);

    fclose(fin);

    free(z);
    free(q);
    free(R);
    free(S);
    free(P1);
    free(C);
    free(header);
    free(P);
}

void decriptare(char *nume_img_criptata,char *nume_img_decriptata,char *nume_fisier_cheie)
{
    FILE *fin,*fc;

    fin=fopen(nume_img_criptata,"rb");
    fc=fopen(nume_fisier_cheie,"r");

    if(fin==NULL)
    {
        printf("Nu am putut deschide fisierul care contine imaginea criptata!\n");
        return;
    }

    if(fc==NULL)
    {
        printf("Nu am putut deschide fisierul care contine cheia secreta!\n");
        return;
    }

    unsigned int R0,SV,*R,*S,w_img,h_img,poz,aux;

    fscanf(fc,"%u%u",&R0,&SV);

    fclose(fc);

    unsigned char *header;
    Pixel *C;

    incarcare_img(nume_img_criptata,&C,&header);

    fseek(fin,18,SEEK_SET);
    fread(&w_img,sizeof(int),1,fin);
    fread(&h_img,sizeof(int),1,fin);

    R=(unsigned int*)malloc(2*w_img*h_img*sizeof(int));

    if(R==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul de numere aleatoare!\n");
        return;
    }

    int i;

    XORSHIFT32(R0,&R,2*w_img*h_img);

    S=(unsigned int*)malloc(w_img*h_img*sizeof(int));

    if(S==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul care contine permutarea!\n");
        return;
    }

    for(i=0;i<w_img*h_img;i++)
        S[i]=i;

    for(i=w_img*h_img-1;i>=1;i--)
    {
        poz=R[w_img*h_img-i]%(i+1);
        aux=S[poz];
        S[poz]=S[i];
        S[i]=aux;
    }

    unsigned int *S1;
    Pixel *C1,*D;

    S1=(unsigned int *)malloc(w_img*h_img*sizeof(int));

    if(S1==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul care contine inversa permutarii!\n");
        return;
    }

    for(i=0;i<w_img*h_img;i++)
        S1[S[i]]=i;

    C1=(Pixel *)malloc(w_img*h_img*sizeof(Pixel));

    if(C1==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul care contine imaginea intermediara!");
        return;
    }

    unsigned char *z,*q;

    z=(unsigned char*)&SV;
    q=(unsigned char*)&R[w_img*h_img];

    C1[0].b=z[0]^C[0].b^q[0];
    C1[0].g=z[1]^C[0].g^q[1];
    C1[0].r=z[2]^C[0].r^q[2];

    for(i=1;i<w_img*h_img;i++)
        {
            z=(unsigned char*)&R[w_img*h_img+i];
            C1[i].b=C[i-1].b^C[i].b^z[0];
            C1[i].g=C[i-1].g^C[i].g^z[1];
            C1[i].r=C[i-1].r^C[i].r^z[2];
        }

    D=(Pixel *)malloc(w_img*h_img*sizeof(Pixel));

    if(D==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorul care contine imaginea decriptata!\n");
        return;
    }

    for(i=0;i<w_img*h_img;i++)
        memcpy(D+S1[i],C1+i,sizeof(Pixel));

    export_img(nume_img_decriptata,D,header);

    fclose(fin);

    free(z);
    free(q);
    free(C);
    free(R);
    free(S);
    free(S1);
    free(C1);
    free(D);
    free(header);
}

void test_chi_patrat(char *nume_imagine_test)
{
    FILE *f=fopen(nume_imagine_test,"rb");

    if(f==NULL)
    {
        printf("Nu am putut deschide fisierul care contine imaginea careia trebuie sa ii fac testul!\n");
        return;
    }

    unsigned int w_img,h_img,i,j,padding;
    unsigned char RGB[3];

    fseek(f,18,SEEK_SET);
    fread(&w_img,sizeof(int),1,f);
    fread(&h_img,sizeof(int),1,f);

    if(w_img%4!=0)
        padding=4-(3*w_img)%4;
    else
        padding=0;

    double f_bar,squareCHI[3];

    f_bar=(w_img*h_img)/256.0;

    fseek(f,54,SEEK_SET);

    unsigned int *blue,*green,*red;

    blue=(unsigned int*)malloc(256*sizeof(unsigned int));
    green=(unsigned int*)malloc(256*sizeof(unsigned int));
    red=(unsigned int*)malloc(256*sizeof(unsigned int));

    if(red==NULL||green==NULL||blue==NULL)
    {
        printf("Eroare la alocarea memoriei pentru vectorii de frecventa din cadrul testului chi-patrat!\n");
        return;
    }

    for(i=0;i<256;i++)
        red[i]=green[i]=blue[i]=0;

    for(i=0;i<h_img;i++)
    {
        for(j=0;j<w_img;j++)
        {
            fread(RGB,3,1,f);
            blue[RGB[0]]++;
            green[RGB[1]]++;
            red[RGB[2]]++;
        }
        fseek(f,padding,SEEK_CUR);
    }

    squareCHI[0]=squareCHI[1]=squareCHI[2]=0;

    for(i=0;i<256;i++)
      {
          squareCHI[0]+=(red[i]-f_bar)*(red[i]-f_bar)/f_bar;
          squareCHI[1]+=(green[i]-f_bar)*(green[i]-f_bar)/f_bar;
          squareCHI[2]+=(blue[i]-f_bar)*(blue[i]-f_bar)/f_bar;
      }

    printf("Valorile testului chi-patrat pentru imaginea %s sunt:\n",nume_imagine_test);

    printf("Rosu: %.2lf\nVerde: %.2lf\nAlbastru: %.2lf\n",squareCHI[0],squareCHI[1],squareCHI[2]);

    free(red);
    free(green);
    free(blue);

    fclose(f);
}
int main()
{
    char *nume_img_init,*nume_img_criptata,*nume_fisier_cheie,*nume_img_decriptata;

    printf("Numele imaginii initiale: ");
    nume_img_init=(char *)malloc(100*sizeof(char));
    if(nume_img_init==NULL)
    {
        printf("Eroare la alocarea spatiului pentru numele imaginii initiale!\n");
        return 0;
    }
    fgets(nume_img_init,100,stdin);

    nume_img_init[strlen(nume_img_init)-1]='\0';

    printf("Numele imaginii criptate: ");
    nume_img_criptata=(char *)malloc(100*sizeof(char));
    if(nume_img_criptata==NULL)
    {
        printf("Eroare la alocarea spatiului pentru numele imaginii criptate!\n");
        return 0;
    }
    fgets(nume_img_criptata,100,stdin);

    nume_img_criptata[strlen(nume_img_criptata)-1]='\0';

    printf("Numele fisierului care contine cheia secreta: ");
    nume_fisier_cheie=(char *)malloc(100*sizeof(char));
    if(nume_fisier_cheie==NULL)
    {
        printf("Eroare la alocarea spatiului pentru numele fisierul care contine cheia secreta!\n");
        return 0;
    }
    fgets(nume_fisier_cheie,100,stdin);

    nume_fisier_cheie[strlen(nume_fisier_cheie)-1]='\0';

    printf("Numele imaginii decriptate: ");
    nume_img_decriptata=(char *)malloc(100*sizeof(char));
    if(nume_img_decriptata==NULL)
    {
        printf("Eroare la alocarea spatiului pentru numele imaginii decriptate!\n");
        return 0;
    }
    fgets(nume_img_decriptata,100,stdin);

    nume_img_decriptata[strlen(nume_img_decriptata)-1]='\0';

    criptare(nume_img_init,nume_img_criptata,nume_fisier_cheie);

    decriptare(nume_img_criptata,nume_img_decriptata,nume_fisier_cheie);

    test_chi_patrat(nume_img_init);

    test_chi_patrat(nume_img_criptata);

    free(nume_img_init);
    free(nume_img_criptata);
    free(nume_fisier_cheie);
    free(nume_img_decriptata);

    return 0;
}
