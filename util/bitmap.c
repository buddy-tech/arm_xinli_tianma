#include <stdio.h>
#include "math.h"

#define WIDTHBYTES(bits) (((bits)+31)/32*4)

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef  int INT;

typedef struct tagBITMAPFILEHEADER
{
    WORD bfType;//固定为0x4d42;
    DWORD bfSize; //文件大小
    WORD bfReserved1; //保留字，不考虑
    WORD bfReserved2; //保留字，同上
    DWORD bfOffBits; //实际位图数据的偏移字节数，即前三个部分长度之和
} BITMAPFILEHEADER;

/*BITMAPFILEHEADER的第1个属性是bfType(2字节)，这里恒定等于0x4D42。由于内存中的数据排列高位在左，低位在右，所以内存中从左往右看就显示成(42 4D)，所以在winhex中头两个 字节显示为(42 4D)就是这样形成的，以后的数据都是这个特点，不再作重复说明。

BITMAPFILEHEADER的第2个属性是bfSize(4字节)，表示整个bmp文件的大小。

BITMAPFILEHEADER的第3个、第4个属性分别是bfReserved1、bfReserved2(各2字节)，这里是2个保留属性，都为0，这里等于&H0000、0x0000。

BITMAPFILEHEADER的第5个属性是bfOffBits(4字节)，表示DIB数据区在bmp文件中的位置偏移量，比如等于0x00000076=118，表示数据区从文件开始往后数的118字节开始。

 BITMAPFILEHEADER结构体这里就讲完了，大家会发现BITMAPFILEHEADER只占了bmp文件开始的14字节长度，但需要 特别说明的是：我们在编程时，经常是以二进制的形式打开一个bmp文件，然后将其开头的14个字节读入自己定义的BITMAPFILEHEADER结构体中，如果按上述定义结构体的方式，需要注意，这个自己定义的结构体在内存中由于字节对齐，会占用16字节的空间，因而直接读入16字节，按字节顺序赋值给结构体，出来的数据是错误的，这样的话，可以先将这14字节的数据读入到一个缓冲器中，然后从缓冲器中按字节对结构体进行赋值。详细程序见后附录。鉴于此中原因，在vb中定义一个BITMAPFILEHEADER结构体变量，其长度占了16个字节，原因就是第1个属性本来应该只分配2个字节，但实际被 分配了4个字节，多出来2个字节，所以如果想保存一张bmp图片，写入BITMAPFILEHEADER结构体时一定要注意这一点。

接下来是BITMAPINFO结构体部分。BITMAPINFO段由两部分组成：BITMAPINFOHEADER结构体和RGBQUAD结构 体。其中RGBQUAD结构体表示图片的颜色信息，有些时候可以省略，一般的24位图片和32位图片都不带RGBQUAD结构体，因为DIB数据区直接表 示的RGB值，一般4位图片和8位图片才带有RGBQUAD结构体。(多少位的图片就是用多少位来表示一个颜色信息，例如4位图片表示用4个bit来表示 一个颜色信息。)一个bmp文件中有没有RGBQUAD结构体，可以根据前面BITMAPFILEHEADER结构体的第5个属性bfOffBits来判 断，因为BITMAPINFOHEADER结构体长度为40bit，如果BITMAPINFOHEADER结构体结束后还未到DIB数据区的偏移量，就说 明接下来的数据是RGBQUAD结构体部分。

下面进入正题BITMAPINFOHEADER部分。*/

typedef struct tagBITMAPINFOHEADER{
 //public:
 DWORD biSize; //指定此结构体的长度，为40
 INT biWidth; //位图宽
 INT biHeight; //位图高
 WORD biPlanes; //平面数，为1
 WORD biBitCount; //采用颜色位数，可以是1，2，4，8，16，24，新的可以是32
 DWORD biCompression; //压缩方式，可以是0，1，2，其中0表示不压缩
 DWORD biSizeImage; //实际位图数据占用的字节数
 INT biXPelsPerMeter; //X方向分辨率
 INT biYPelsPerMeter; //Y方向分辨率
 DWORD biClrUsed; //使用的颜色数，如果为0，则表示默认值(2^颜色位数)
 DWORD biClrImportant; //重要颜色数，如果为0，则表示所有颜色都是重要的
} BITMAPINFOHEADER;

/*BITMAPINFOHEADER的第1个属性是biSize(4字节)，表示BITMAPINFOHEADER结构体的长度，最常见的长度是40字节。

BITMAPINFOHEADER的第2个属性是biWidth(4字节)，表示bmp图片的宽度

BITMAPINFOHEADER的第3个属性是biHeight(4字节)，表示bmp图片的高度

BITMAPINFOHEADER的第4个属性是biPlanes(2字节)，表示bmp图片的平面属，显然显示器只有一个平面，所以恒等于1，这里等于0x0001。

BITMAPINFOHEADER的第5个属性是biBitCount(2字节)，表示bmp图片的颜色位数，即1位图（单色或二值图像），8位图，16位图，24位图、32位图等等。

BITMAPINFOHEADER的第6个属性是biCompression(4字节)，表示图片的压缩属性，bmp图片是不压缩的，等于0，所以这里为0x00000000。

BITMAPINFOHEADER的第7个属性是biSizeImage(4字节)，表示bmp图片数据区的大小，当上一个数值biCompression等于0时，这里的值可以省略不填，所以这里等于0x00000000。

BITMAPINFOHEADER的第8个属性是biXPelsPerMeter(4字节)，表示图片X轴每米多少像素，可省略，这里等于0x00000EC3=3779像素/米。

BITMAPINFOHEADER的第9个属性是biYPelsPerMeter(4字节)，表示图片Y轴每米多少像素，可省略，这里等于0x00000EC3=3779像素/米。

BITMAPINFOHEADER的第10个属性是biClrUsed(4字节)，表示使用了多少个颜色索引表，一般biBitCount属性小于16才会用到，等于0时表示有2^biBitCount个颜色索引表，所以这里仍等于0x00000000。

BITMAPINFOHEADER的第11个属性是biClrImportant(4字节)，表示有多少个重要的颜色，等于0时表示所有颜色都很重要，所以这里等于0x00000000。

至此BITMAPINFOHEADER结构体结束。

由于这个图片到这里还未到达DIB数据区的偏移量，或者说由于BITMAPINFOHEADER的第5个属性是biBitCount<16，也就是在1位图（只有两种颜色），4位图（只有2^4=16种颜色），8位图（只有2^8=256种颜色）的情况下，此时会有颜色表，也就是接下来的部分：RGBQUAD结构体。

//调色板Palette，当然，这里是对那些需要调色板的位图文件而言的。24位和32位是不需要调色板的。
//（调色板结构体个数等于使用的颜色数，即是多少色图就有多少个，4位图16色，就有16个RGBQUAD结构体。）
*/

typedef struct tagRGBQUAD {
//public:
BYTE rgbBlue; //该颜色的蓝色分量
BYTE rgbGreen; //该颜色的绿色分量
BYTE rgbRed; //该颜色的红色分量
BYTE rgbReserved; //保留值
} RGBQUAD;

/*这里举个4位图也就是16色图的例子：一 个RGBQUAD结构体只占用4字节空间，从左到右每个字节依次表示(蓝色，绿色，红色，未使用)。举例的这个图片我数了数总共有16个RGBQUAD结 构体，由于该图片是4位图，2^4正好等于16，所以它把16种颜色全部都枚举出来了，这些颜色就是一个颜色索引表。颜色索引表编号从0开始，总共16个 颜色，所以编号为0-15。从winhex中可以看到按照顺序，这16个RGBQUAD结构体依次为：

编号：(蓝，绿，红，空)

0号：(00，00，00，00)

1号：(00，00，80，00)

2号：(00，80，00，00)

3号：(00，80，80，00)

4号：(80，00，00，00)

5号：(80，00，80，00)

6号：(80，80，00，00)

7号：(80，80，80，00)

8号：(C0，C0，C0，00)

9号：(00，00，FF，00)

10号：(00，FF，00，00)

11号：(00，FF，FF，00)

12号：(FF，00，00，00)

13号：(FF，00，FF，00)

14号：(FF，FF，00，00)

15号：(FF，FF，FF，00)

到这里，正好满足DIB数据区的偏移量，所以后面的字节就是图片内容了。这里需要提醒的是所有的DIB数据扫描行是上下颠倒的，也就是说一幅图片先绘制底部的像素，再绘制顶部的像素，所以这些DIB数据所表示的像素点就是从图片的左下角开始，一直表示到图片的右上角。
*/

//ReadBitMap
//

static void showBmpHead(BITMAPFILEHEADER* pBmpHead)
{
    printf("位图文件头:\n");
    printf("bmp格式标志bftype：0x%x\n",pBmpHead->bfType );
    printf("文件大小:%d\n",pBmpHead->bfSize);
    printf("保留字:%d\n",pBmpHead->bfReserved1);
    printf("保留字:%d\n",pBmpHead->bfReserved2);
    printf("实际位图数据的偏移字节数:%d\n",pBmpHead->bfOffBits);
}


static void showBmpInforHead(BITMAPINFOHEADER* pBmpInforHead)
{
    printf("位图信息头:\n");
    printf("结构体的长度:%d\n",pBmpInforHead->biSize);
    printf("位图宽:%d\n",pBmpInforHead->biWidth);
    printf("位图高:%d\n",pBmpInforHead->biHeight);
    printf("biPlanes平面数:%d\n",pBmpInforHead->biPlanes);
    printf("biBitCount采用颜色位数:%d\n",pBmpInforHead->biBitCount);
    printf("压缩方式:%d\n",pBmpInforHead->biCompression);
    printf("biSizeImage实际位图数据占用的字节数:%d\n",pBmpInforHead->biSizeImage);
    printf("X方向分辨率:%d\n",pBmpInforHead->biXPelsPerMeter);
    printf("Y方向分辨率:%d\n",pBmpInforHead->biYPelsPerMeter);
    printf("使用的颜色数:%d\n",pBmpInforHead->biClrUsed);
    printf("重要颜色数:%d\n",pBmpInforHead->biClrImportant);
}

static void showRgbQuan(RGBQUAD* pRGB)
{
    printf("(%-3d,%-3d,%-3d) ",pRGB->rgbRed,pRGB->rgbGreen,pRGB->rgbBlue);
}

int parseBmp(char *pBmpName,char **ppRGBData,int *pSize)
{
    int i,j;
    BITMAPFILEHEADER bitHead;
    BITMAPINFOHEADER bitInfoHead;
    FILE* pfile;

    char *BmpFileHeader;
    WORD *temp_WORD;
    DWORD *temp_DWORD;

    pfile = fopen(pBmpName,"rb");//打开文件
    BmpFileHeader=(char *)malloc(14*sizeof(char));
    if(pfile!=NULL)
    {
        printf("file %s open success.\n",pBmpName);
        //读取位图文件头信息
        fread(BmpFileHeader,1,14,pfile);
        temp_WORD=(WORD* )(BmpFileHeader);
        bitHead.bfType=*temp_WORD;
        if(bitHead.bfType != 0x4d42)
        {
            printf("file is not .bmp file!");
            return -1;
        }
        temp_DWORD=(DWORD *)(BmpFileHeader+sizeof(bitHead.bfType));
        bitHead.bfSize=*temp_DWORD;
        temp_WORD=(WORD*)(BmpFileHeader+sizeof(bitHead.bfType)+sizeof(bitHead.bfSize));
        bitHead.bfReserved1=*temp_WORD;
        temp_WORD=(WORD*)(BmpFileHeader+sizeof(bitHead.bfType)+sizeof(bitHead.bfSize)+sizeof(bitHead.bfReserved1));
        bitHead.bfReserved2=*temp_WORD;
        temp_DWORD=(DWORD*)(BmpFileHeader+sizeof(bitHead.bfType)+sizeof(bitHead.bfSize)+sizeof(bitHead.bfReserved1)+sizeof(bitHead.bfReserved2));
        bitHead.bfOffBits=*temp_DWORD;

        showBmpHead(&bitHead);
        printf("\n\n");

        //读取位图信息头信息
        fread(&bitInfoHead,1,sizeof(BITMAPINFOHEADER),pfile);
        showBmpInforHead(&bitInfoHead);
        printf("\n");
    }
    else
    {
        printf("file open fail!\n");
        return -2;
    }

    RGBQUAD *pRgb ;

    if(bitInfoHead.biBitCount < 16)//有调色板
    {
        //读取调色盘结信息
        int nPlantNum = (int)pow(2,bitInfoHead.biBitCount); // Mix color Plant Number;
        pRgb=(RGBQUAD *)malloc(nPlantNum*sizeof(RGBQUAD));
        memset(pRgb,0,nPlantNum*sizeof(RGBQUAD));
        int num = fread(pRgb,4,nPlantNum,pfile);

        printf("Color Plate Number: %d\n",nPlantNum);

        printf("颜色板信息:\n");
        for (i =0; i< nPlantNum;i++)
        {
            if (i%5==0)
            {
                printf("\n");
            }
            showRgbQuan(&pRgb[i]);
        }
        printf("\n");
    }

    int width    = (bitInfoHead.biWidth+15)/16*16;
    int orgwidth = bitInfoHead.biWidth;
    int height = bitInfoHead.biHeight;

    //width = orgwidth;
    //分配内存空间把源图存入内存
    int l_width = WIDTHBYTES(orgwidth* bitInfoHead.biBitCount);//计算位图的实际宽度并确保它为32的倍数
    BYTE *pColorData=(BYTE *)malloc(height*l_width);
    memset(pColorData,0,height*l_width);
    INT nData = height*l_width;

    //把位图数据信息读到数组里
    fread(pColorData,1,nData,pfile);
    for(i=0;i<6;i++)
    {
        printf("%x-",pColorData[i]);
    }
    printf("\nxxxxx\n");
    //将位图数据转化为RGB数据
    unsigned char *pBmpRGBHead = malloc(width*height*4 + 64);
    unsigned char *pBmpRGBData = pBmpRGBHead + 64;
    memset(pBmpRGBHead,0,width*height*4+64);
    *pSize = width*height*4 + 64;
    /*for(i=0;i<64;i++)
    {
        pBmpRGBHead[i] = 0;
    }*/
    for(i=0;i<8;i++)
    {
        pBmpRGBHead[i] = i;
    }
    pBmpRGBHead[24] =  height&0xFF;
    pBmpRGBHead[25] = (height&0xFF00)>>8;
    pBmpRGBHead[26] =  width&0xFF;
    pBmpRGBHead[27] = (width&0xFF00)>>8;
    pBmpRGBHead[32] = 32;
    pBmpRGBHead[33] = 0;

    //RGBQUAD* dataOfBmp = (RGBQUAD *)malloc(width*height*sizeof(RGBQUAD));//用于保存各像素对应的RGB数据
    //memset(dataOfBmp,0,width*height*sizeof(RGBQUAD));

    if(bitInfoHead.biBitCount<16)//有调色板，即位图为非真彩色
    {
        int k;
        int index = 0;
        if (bitInfoHead.biBitCount == 1)
        {
            for(i=0;i<height;i++)
               for(j=0;j<8*width;j++)
               {
                 BYTE mixIndex= 0;
                 k = i*l_width + j/8;//k:取得该像素颜色数据在实际数据数组中的序号
                 //j:提取当前像素的颜色的具体值
                 mixIndex = pColorData[k];
                 switch(j%8)
                 {
                     case 0:
                      mixIndex = mixIndex<<7;
                      mixIndex = mixIndex>>7;
                      break;
                     case 1:
                      mixIndex = mixIndex<<6;
                      mixIndex = mixIndex>>7;
                      break;
                     case 2:
                      mixIndex = mixIndex<<5;
                      mixIndex = mixIndex>>7;
                      break;

                     case 3:
                      mixIndex = mixIndex<<4;
                      mixIndex = mixIndex>>7;
                      break;
                     case 4:
                      mixIndex = mixIndex<<3;
                      mixIndex = mixIndex>>7;
                      break;

                     case 5:
                      mixIndex = mixIndex<<2;
                      mixIndex = mixIndex>>7;
                      break;
                     case 6:
                      mixIndex = mixIndex<<1;
                      mixIndex = mixIndex>>7;
                      break;

                     case 7:
                      mixIndex = mixIndex>>7;
                      break;
                 }
                 //将像素数据保存到数组中对应的位置
                 //dataOfBmp[index].rgbRed = pRgb[mixIndex].rgbRed;
                 //dataOfBmp[index].rgbGreen = pRgb[mixIndex].rgbGreen;
                 //dataOfBmp[index].rgbBlue = pRgb[mixIndex].rgbBlue;
                 //dataOfBmp[index].rgbReserved = pRgb[mixIndex].rgbReserved;
                 pBmpRGBData[index*4+0] = pRgb[mixIndex].rgbRed;
                 pBmpRGBData[index*4+1] = pRgb[mixIndex].rgbGreen;
                 pBmpRGBData[index*4+2] = pRgb[mixIndex].rgbGreen;
                 index++;
               }
        }

        if(bitInfoHead.biBitCount==2)
        {
            for(i=0;i<height;i++)
               for(j=0;j<4*width;j++)
               {
                 BYTE mixIndex= 0;
                 k = i*l_width + j/4;//k:取得该像素颜色数据在实际数据数组中的序号
                 //j:提取当前像素的颜色的具体值
                 mixIndex = pColorData[k];
                 switch(j%4)
                 {
                     case 0:
                      mixIndex = mixIndex<<6;
                      mixIndex = mixIndex>>6;
                      break;
                     case 1:
                      mixIndex = mixIndex<<4;
                      mixIndex = mixIndex>>6;
                      break;
                     case 2:
                      mixIndex = mixIndex<<2;
                      mixIndex = mixIndex>>6;
                      break;
                     case 3:
                      mixIndex = mixIndex>>6;
                      break;
                 }
                 //将像素数据保存到数组中对应的位置
                 //dataOfBmp[index].rgbRed = pRgb[mixIndex].rgbRed;
                 //dataOfBmp[index].rgbGreen = pRgb[mixIndex].rgbGreen;
                 //dataOfBmp[index].rgbBlue = pRgb[mixIndex].rgbBlue;
                 //dataOfBmp[index].rgbReserved = pRgb[mixIndex].rgbReserved;
                 pBmpRGBData[index*4+0] = pRgb[mixIndex].rgbRed;
                 pBmpRGBData[index*4+1] = pRgb[mixIndex].rgbGreen;
                 pBmpRGBData[index*4+2] = pRgb[mixIndex].rgbGreen;
                 index++;
            }
        }
        if(bitInfoHead.biBitCount == 4)
        {
            for(i=0;i<height;i++)
               for(j=0;j<2*width;j++)
                {
                 BYTE mixIndex= 0;
                 k = i*l_width + j/2;
                 mixIndex = pColorData[k];
                 if(j%2==0)
                 {//低
                  mixIndex = mixIndex<<4;
                  mixIndex = mixIndex>>4;
                 }
                 else
                 {//高
                  mixIndex = mixIndex>>4;
                 }

                 //dataOfBmp[index].rgbRed = pRgb[mixIndex].rgbRed;
                 //dataOfBmp[index].rgbGreen = pRgb[mixIndex].rgbGreen;
                 //dataOfBmp[index].rgbBlue = pRgb[mixIndex].rgbBlue;
                 //dataOfBmp[index].rgbReserved = pRgb[mixIndex].rgbReserved;
                 pBmpRGBData[index*4+0] = pRgb[mixIndex].rgbRed;
                 pBmpRGBData[index*4+1] = pRgb[mixIndex].rgbGreen;
                 pBmpRGBData[index*4+2] = pRgb[mixIndex].rgbGreen;
                 index++;
               }
        }
        if(bitInfoHead.biBitCount == 8)
        {
            for(i=0;i<height;i++)
               for(j=0;j<width;j++)
                {
                 BYTE mixIndex= 0;

                 k = i*l_width + j;

                 mixIndex = pColorData[k];

                 //dataOfBmp[index].rgbRed = pRgb[mixIndex].rgbRed;
                 //dataOfBmp[index].rgbGreen = pRgb[mixIndex].rgbGreen;
                 //dataOfBmp[index].rgbBlue = pRgb[mixIndex].rgbBlue;
                 //dataOfBmp[index].rgbReserved = pRgb[mixIndex].rgbReserved;
                 pBmpRGBData[index*4+0] = pRgb[mixIndex].rgbRed;
                 pBmpRGBData[index*4+1] = pRgb[mixIndex].rgbGreen;
                 pBmpRGBData[index*4+2] = pRgb[mixIndex].rgbBlue;
                 index++;
                }
        }
    }
    else if(bitInfoHead.biBitCount == 16)
    {}
    else if(bitInfoHead.biBitCount == 24)//位图为24位真彩色
    {
        int k;
        int index = 0;
        for(i=0;i<height;i++)
        {
            for(j=0;j<orgwidth;j++)
            {
                k = i*l_width + j*3;
                //dataOfBmp[index].rgbRed   = pColorData[k+2];
                //dataOfBmp[index].rgbGreen = pColorData[k+1];
                //dataOfBmp[index].rgbBlue  = pColorData[k];
                pBmpRGBData[index*4+2] = pColorData[k+2];
                pBmpRGBData[index*4+1] = pColorData[k+1];
                pBmpRGBData[index*4+0] = pColorData[k];
                index++;
            }
            for(j=orgwidth;j<width;j++)
            {
                index++;
            }
            if(index<10000)
            printf("index:%d %x %x %x\n",index,pBmpRGBData[i*4+0],pBmpRGBData[i*4+1],pBmpRGBData[i*4+2]);
        }
    }
    else//位图为32位真彩色
    {
        int k;
        int index = 0;
        for(i=0;i<height;i++)
        {
            for(j=0;j<orgwidth;j++)
            {
                k = i*l_width + j*4;
                //dataOfBmp[index].rgbRed   = pColorData[k+2];
                //dataOfBmp[index].rgbGreen = pColorData[k+1];
                //dataOfBmp[index].rgbBlue  = pColorData[k];
                pBmpRGBData[index*4+0] = pColorData[k+2];
                pBmpRGBData[index*4+1] = pColorData[k+1];
                pBmpRGBData[index*4+2] = pColorData[k];
                index++;
            }
            for(j=orgwidth;j<width;j++)
            {
                index++;
            }
        }
    }
    printf("像素数据信息:\n");

    fclose(pfile);

    if (bitInfoHead.biBitCount<16)
    {
        free(pRgb);
    }
    *ppRGBData = pBmpRGBHead;
    /*{
        int ii;
        for(ii=0;ii<128;ii++)
        {
            if(ii%16 == 0)
                printf("\n");
            printf("%x-",pBmpRGBHead[ii]);
        }
        printf("\n");
    }*/
    printf("org picture width:%d ptn width:%d\n",orgwidth,width);
    //free(dataOfBmp);
    free(pColorData);
    free(BmpFileHeader);
    return 0;
}

#define PTN_FILE_PATH  "cfg/ptn/"

//save bmp to ptn dir with the sameName
void saveBmpToPtn(char *pBmpFullName)
{
    unsigned char *pRgbBuffer;
    unsigned int  u32RgbSize;
    unsigned  char tmpDir[64];
    unsigned  char ptnDir[128];
    char *pStr = strstr(pBmpFullName,"imageOrigin");
    if(!pStr)
    {
        return;
    }
    pStr += strlen("imageOrigin");
    char *pStr2 = strrchr(pBmpFullName,'/');
    memset(tmpDir,0,sizeof(tmpDir));
    memcpy(tmpDir,pStr,pStr2-pStr);
    sprintf(ptnDir,"%s/%s",PTN_FILE_PATH,tmpDir);
    mkdir_recursive(ptnDir);
    printf("ptnDir is %s\n",ptnDir);

    if(parseBmp(pBmpFullName,&pRgbBuffer,&u32RgbSize) == 0)
    {
        FILE* pfile;

        unsigned  char ptnFullName[128];
        unsigned  char bmpBaseName[64];

        pStr2++;
        sprintf(bmpBaseName,"%s",pStr2);
        sprintf(ptnFullName,"%s/%s",ptnDir,bmpBaseName);
        pfile = fopen(ptnFullName,"wb");//打开文件
        if(!pfile)
        {
            return;
        }
        fwrite(pRgbBuffer,1,u32RgbSize,pfile);
        fclose(pfile);
        free(pRgbBuffer);
        system("sync");
    }
}

void loadPtnToMem(char *pBmpFullName,char **ppRgbBuffer,int *u32RgbSize)
{
    FILE* pfile;
    int fileSize;
    char *pRgbBuffer;
    unsigned  char bmpBaseName[64];
    unsigned  char ptnFullName[128];
    char *pStr = strstr(pBmpFullName,"imageOrigin");
    if(!pStr)
        return;

    char *pStr2 = strchr(pStr,'/');
    if(!pStr2)
    {
        printf("loadPtnToMem:error at strrchr \n");
        return;
    }

    pStr2++;
    sprintf(bmpBaseName,"%s",pStr2);
    sprintf(ptnFullName,"%s/%s",PTN_FILE_PATH,bmpBaseName);
    pfile = fopen(ptnFullName,"rb");//打开文件
    if(!pfile)
    {
        printf("loadPtnToMem:error at open file %s \n",ptnFullName);
        return;
    }

    fseek(pfile,0,SEEK_END);
    fileSize = ftell(pfile);
    fseek(pfile,0,0);
    pRgbBuffer = malloc(fileSize);
    fread(pRgbBuffer,1,fileSize,pfile);
    fclose(pfile);
    *u32RgbSize  = fileSize;
    *ppRgbBuffer = pRgbBuffer;
    return;
}
