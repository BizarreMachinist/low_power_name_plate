#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
// #include "pico/stdio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include "./images.h"



#define DEFAULT_WAIT 1*1000

#define POT_OUT_PIN 13
#define POT_IN_PIN 26
#define POT_IN_PIN_ADC 0

#define LIGHT_OUT_PIN 15
#define LIGHT_IN_PIN 14

#define SCRN_PWR 12
#define SCRN_TX 19
#define SCRN_SCLK 18
#define SCRN_CS 17
#define SCRN_DC 20
#define SCRN_RST 21
#define SCRN_BUSY 22

#define GPIO_FUNC_SPI 1

#define EPD_2IN13B_V3_WIDTH 104
#define EPD_2IN13B_V3_HEIGHT 212
#define WHITE 0xFF
#define BLACK 0x00



void blink(uint8_t);
uint16_t get_pot(void);
bool is_light(void);
void single_blink(uint16_t);
void scrn_test(uint8_t);

static void EPD_2IN13B_V3_Reset(void);
static void EPD_2IN13B_V3_SendCommand(const uint8_t* Reg);
static void EPD_2IN13B_V3_SendData(const uint8_t* Data);
void EPD_2IN13B_V3_ReadBusy(void);
static void EPD_2IN13B_V3_TurnOnDisplay(void);
void EPD_2IN13B_V3_Init(void);
void EPD_2IN13B_V3_Clear(void);
void EPD_2IN13B_V3_Display(const uint8_t*, const uint8_t*);
void EPD_2IN13B_V3_Sleep(void);

void Paint_NewImage(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t);
void Paint_SelectImage(uint8_t*);
void Paint_Clear(uint16_t);
void Paint_SetPixel(uint16_t, uint16_t, uint16_t);

uint8_t GUI_ReadBmp(const char*, uint16_t, uint16_t);

void DEV_Digital_Write(uint16_t, uint8_t);
uint8_t DEV_Digital_Read(uint16_t);
void DEV_SPI_WriteByte(const uint8_t*);
void DEV_Delay_ms(uint32_t);
void DEV_Module_Exit(void);

typedef struct {
    uint8_t *Image;
    uint16_t Width;
    uint16_t Height;
    uint16_t WidthMemory;
    uint16_t HeightMemory;
    uint16_t Color;
    uint16_t Rotate;
    uint16_t Mirror;
    uint16_t WidthByte;
    uint16_t HeightByte;
    uint16_t Scale;
} PAINT;
PAINT Paint;

/*Bitmap file header   14bit*/
typedef struct BMP_FILE_HEADER {
    uint16_t bType;        //File identifier - BM
    uint32_t bSize;      //The size of the file - 90
    uint16_t bReserved1;   //Reserved value, must be set to 0 - 0
    uint16_t bReserved2;   //Reserved value, must be set to 0 - 0
    uint32_t bOffset;    //The offset from the beginning of the file header to the beginning of the image data bit - 54
}
__attribute__ ((packed)) BMPFILEHEADER;    // 14bit

/*Bitmap information header  40bit*/
typedef struct BMP_INFO {
    uint32_t biInfoSize;      //The size of the header - 40
    uint32_t biWidth;         //The width of the image - 3
    uint32_t biHeight;        //The height of the image - 3
    uint16_t biPlanes;          //The number of planes in the image - 1
    uint16_t biBitCount;        //The number of bits per pixel - 24
    uint32_t biCompression;   //Compression type - 0
    uint32_t bimpImageSize;   //The size of the image, in bytes - 36
    uint32_t biXPelsPerMeter; //Horizontal resolution - 3780
    uint32_t biYPelsPerMeter; //Vertical resolution - 3780
    uint32_t biClrUsed;       //The number of colors used - 0
    uint32_t biClrImportant;  //The number of important colors - 0
}
__attribute__ ((packed)) BMPINFOHEADER;

/*Color table: palette */
typedef struct RGB_QUAD {
    uint8_t rgbBlue;               //Blue intensity
    uint8_t rgbGreen;              //Green strength
    uint8_t rgbRed;                //Red intensity
    uint8_t rgbReversed;           //Reserved value
}
__attribute__ ((packed)) BMPRGBQUAD;



void blink(uint8_t cycles)
{
    for(uint8_t i=0; i<cycles; i+=1)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(500);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(500);
    }
}

void single_blink(uint16_t wait)
{
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(wait);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    sleep_ms(wait);
}

uint16_t get_pot(void)
{
    uint16_t val = adc_read();
    printf("%u\n", val);
    return val;
}

bool is_light(void)
{
    return gpio_get(LIGHT_IN_PIN);
}

void scrn_test(uint8_t img)
{
    gpio_put(SCRN_PWR, 1);
    EPD_2IN13B_V3_Init();
    EPD_2IN13B_V3_Clear();
    DEV_Delay_ms(500);

    uint8_t* BLayer;
    uint8_t* RLayer;
    uint32_t imageSize = ((EPD_2IN13B_V3_WIDTH % 8 == 0)? (EPD_2IN13B_V3_WIDTH / 8 ): (EPD_2IN13B_V3_WIDTH / 8 + 1)) * EPD_2IN13B_V3_HEIGHT;

    if((BLayer = (uint8_t*)malloc(imageSize)) == NULL) blink(7);
    if((RLayer = (uint8_t*)malloc(imageSize)) == NULL) blink(5);

    Paint_NewImage(BLayer, EPD_2IN13B_V3_WIDTH, EPD_2IN13B_V3_HEIGHT, 270, WHITE);
    Paint_NewImage(RLayer, EPD_2IN13B_V3_WIDTH, EPD_2IN13B_V3_HEIGHT, 270, WHITE);

    Paint_SelectImage(BLayer);
    Paint_Clear(WHITE);
    Paint_SelectImage(RLayer);
    Paint_Clear(WHITE);

    EPD_2IN13B_V3_Display(coalition[img], coalition[img+1]);
    DEV_Delay_ms(2000);

    EPD_2IN13B_V3_Sleep();
    free(BLayer);
    free(RLayer);
    BLayer = NULL;
    RLayer = NULL;
    DEV_Delay_ms(2000);

    printf("now stop\n");

    DEV_Module_Exit();
    gpio_put(SCRN_PWR, 0);
}

//===============================================

void Paint_NewImage(uint8_t* image, uint16_t Width, uint16_t Height, uint16_t Rotate, uint16_t Color)
{
    Paint.Image = NULL;
    Paint.Image = image;

    Paint.WidthMemory = Width;
    Paint.HeightMemory = Height;
    Paint.Color = Color;    
    Paint.Scale = 2;
    Paint.WidthByte = (Width % 8 == 0)? (Width / 8 ): (Width / 8 + 1);
    Paint.HeightByte = Height;    

    Paint.Rotate = Rotate;
    Paint.Mirror = 0x00;

    if(Rotate == 0 || Rotate == 180) {
        Paint.Width = Width;
        Paint.Height = Height;
    } else {
        Paint.Width = Height;
        Paint.Height = Width;
    }
}

void Paint_SelectImage(uint8_t* image)
{
    Paint.Image = image;
}

void Paint_Clear(uint16_t Color)
{
    if(Paint.Scale == 2) {
        for (uint16_t Y = 0; Y < Paint.HeightByte; Y++) {
            for (uint16_t X = 0; X < Paint.WidthByte; X++ ) {//8 pixel =  1 byte
                uint32_t Addr = X + Y*Paint.WidthByte;
                Paint.Image[Addr] = Color;
            }
        }
    }else if(Paint.Scale == 4) {
        for (uint16_t Y = 0; Y < Paint.HeightByte; Y++) {
            for (uint16_t X = 0; X < Paint.WidthByte; X++ ) {
                uint32_t Addr = X + Y*Paint.WidthByte;
                Paint.Image[Addr] = (Color<<6)|(Color<<4)|(Color<<2)|Color;
            }
        }
    }else if(Paint.Scale == 7 || Paint.Scale == 16) {
        for (uint16_t Y = 0; Y < Paint.HeightByte; Y++) {
            for (uint16_t X = 0; X < Paint.WidthByte; X++ ) {
                uint32_t Addr = X + Y*Paint.WidthByte;
                Paint.Image[Addr] = (Color<<4)|Color;
            }
        }
    }
}

void Paint_SetPixel(uint16_t Xpoint, uint16_t Ypoint, uint16_t Color)
{
    if(Xpoint > Paint.Width || Ypoint > Paint.Height) return;

    uint16_t X, Y;
    switch(Paint.Rotate) {
    case 0:
        X = Xpoint;
        Y = Ypoint;
        break;
    case 90:
        X = Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;
    default:
        return;
    }

    switch(Paint.Mirror) {
    case 0x00:
        break;
    case 0x01:
        X = Paint.WidthMemory - X - 1;
        break;
    case 0x02:
        Y = Paint.HeightMemory - Y - 1;
        break;
    case 0x03:
        X = Paint.WidthMemory - X - 1;
        Y = Paint.HeightMemory - Y - 1;
        break;
    default:
        return;
    }

    if(X > Paint.WidthMemory || Y > Paint.HeightMemory) return;

    if(Paint.Scale == 2){
        uint32_t Addr = X / 8 + Y * Paint.WidthByte;
        uint8_t Rdata = Paint.Image[Addr];
        if(Color == BLACK)
            Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
        else
            Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
    }else if(Paint.Scale == 4){
        uint32_t Addr = X / 4 + Y * Paint.WidthByte;
        Color = Color % 4;//Guaranteed color scale is 4  --- 0~3
        uint8_t Rdata = Paint.Image[Addr];
        Rdata = Rdata & (~(0xC0 >> ((X % 4)*2)));//Clear first, then set value
        Paint.Image[Addr] = Rdata | ((Color << 6) >> ((X % 4)*2));
    }else if(Paint.Scale == 7 || Paint.Scale == 16){
        uint32_t Addr = X / 2  + Y * Paint.WidthByte;
        uint8_t Rdata = Paint.Image[Addr];
        Rdata = Rdata & (~(0xF0 >> ((X % 2)*4)));//Clear first, then set value
        Paint.Image[Addr] = Rdata | ((Color << 4) >> ((X % 2)*4));
        // printf("Add =  %d ,data = %d\r\n",Addr,Rdata);
    }
}

uint8_t GUI_ReadBmp(const char* path, uint16_t Xstart, uint16_t Ystart)
{
    FILE* fp;                     //Define a file pointer
    BMPFILEHEADER bmpFileHeader;  //Define a bmp file header structure
    BMPINFOHEADER bmpInfoHeader;  //Define a bmp info header structure


    if((fp = fopen(path, "rb")) == NULL) {
        exit(0);
    }

    // Set the file pointer from the beginning
    fseek(fp, 0, SEEK_SET);
    fread(&bmpFileHeader, sizeof(BMPFILEHEADER), 1, fp);    //sizeof(BMPFILEHEADER) must be 14
    fread(&bmpInfoHeader, sizeof(BMPINFOHEADER), 1, fp);    //sizeof(BMPFILEHEADER) must be 50

    uint16_t Image_Width_Byte = (bmpInfoHeader.biWidth % 8 == 0)? (bmpInfoHeader.biWidth / 8): (bmpInfoHeader.biWidth / 8 + 1);
    uint16_t Bmp_Width_Byte = (Image_Width_Byte % 4 == 0) ? Image_Width_Byte: ((Image_Width_Byte / 4 + 1) * 4);
    uint8_t Image[Image_Width_Byte * bmpInfoHeader.biHeight];
    memset(Image, 0xFF, Image_Width_Byte * bmpInfoHeader.biHeight);

    // Determine if it is a monochrome bitmap
    int readbyte = bmpInfoHeader.biBitCount;
    if(readbyte != 1) {
        exit(0);
    }

    // Determine black and white based on the palette
    uint16_t i;
    uint16_t Bcolor, Wcolor;
    uint16_t bmprgbquadsize = pow(2, bmpInfoHeader.biBitCount);// 2^1 = 2
    BMPRGBQUAD bmprgbquad[bmprgbquadsize];        //palette

    for(i = 0; i < bmprgbquadsize; i++){
    // for(i = 0; i < 2; i++) {
        fread(&bmprgbquad[i], sizeof(BMPRGBQUAD), 1, fp);
    }
    if(bmprgbquad[0].rgbBlue == 0xff && bmprgbquad[0].rgbGreen == 0xff && bmprgbquad[0].rgbRed == 0xff) {
        Bcolor = BLACK;
        Wcolor = WHITE;
    } else {
        Bcolor = WHITE;
        Wcolor = BLACK;
    }

    // Read image data into the cache
    uint16_t x, y;
    uint8_t Rdata;
    fseek(fp, bmpFileHeader.bOffset, SEEK_SET);
    for(y = 0; y < bmpInfoHeader.biHeight; y++) {//Total display column
        for(x = 0; x < Bmp_Width_Byte; x++) {//Show a line in the line
            if(fread((char *)&Rdata, 1, readbyte, fp) != readbyte) {
                break;
            }
            if(x < Image_Width_Byte) { //bmp
                Image[x + (bmpInfoHeader.biHeight - y - 1) * Image_Width_Byte] =  Rdata;
            }
        }
    }
    fclose(fp);

    // Refresh the image to the display buffer based on the displayed orientation
    uint8_t color, temp;
    for(y = 0; y < bmpInfoHeader.biHeight; y++) {
        for(x = 0; x < bmpInfoHeader.biWidth; x++) {
            if(x > Paint.Width || y > Paint.Height) {
                break;
            }
            temp = Image[(x / 8) + (y * Image_Width_Byte)];
            color = (((temp << (x%8)) & 0x80) == 0x80) ?Bcolor:Wcolor;
            Paint_SetPixel(Xstart + x, Ystart + y, color);
        }
    }
    return 0;
}

void DEV_Digital_Write(uint16_t Pin, uint8_t Value)
{
    gpio_put(Pin, Value);
}

uint8_t DEV_Digital_Read(uint16_t Pin)
{
    uint8_t Read_value = 0;
    Read_value = gpio_get(Pin);
    return Read_value;
}

void DEV_SPI_WriteByte(const uint8_t* Value)
{
    spi_write_blocking(spi_default, Value, 1);
}

void DEV_Delay_ms(uint32_t xms)
{
    sleep_ms(xms);
}

void DEV_Module_Exit(void)
{
    DEV_Digital_Write(SCRN_CS, 0);
    DEV_Digital_Write(SCRN_PWR, 0);
    DEV_Digital_Write(SCRN_DC, 0);
    DEV_Digital_Write(SCRN_RST, 0);
}

static void EPD_2IN13B_V3_Reset(void)
{
    // DEV_Digital_Write(SCRN_CS, 1);
    
    DEV_Digital_Write(SCRN_RST, 1);
    DEV_Delay_ms(200);
    DEV_Digital_Write(SCRN_RST, 0);
    DEV_Delay_ms(1);
    DEV_Digital_Write(SCRN_RST, 1);
    DEV_Delay_ms(200);
}

static void EPD_2IN13B_V3_SendCommand(const uint8_t* Reg)
{
    DEV_Digital_Write(SCRN_DC, 0);
    DEV_Digital_Write(SCRN_CS, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(SCRN_CS, 1);
}

static void EPD_2IN13B_V3_SendData(const uint8_t* Data)
{
    DEV_Digital_Write(SCRN_DC, 1);
    DEV_Digital_Write(SCRN_CS, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(SCRN_CS, 1);
}

void EPD_2IN13B_V3_ReadBusy(void)
{
    uint8_t busy;
    do{
        DEV_Delay_ms(100);
        //EPD_2IN13B_V3_SendCommand(&(uint8_t){0x71});
        busy = DEV_Digital_Read(SCRN_BUSY);
        busy =!(busy & 0x01);
    }while(busy);
}

static void EPD_2IN13B_V3_TurnOnDisplay(void)
{
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0x12});		 //DISPLAY REFRESH
    DEV_Delay_ms(100);
    EPD_2IN13B_V3_ReadBusy();
}

//================================================
void EPD_2IN13B_V3_Init(void)
{
    printf("1\n");
    EPD_2IN13B_V3_Reset();
    printf("2\n");
    DEV_Delay_ms(10);
    EPD_2IN13B_V3_ReadBusy();
    
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0x04});  
    printf("3\n");
    EPD_2IN13B_V3_ReadBusy();//waiting for the electronic paper IC to release the idle signal

    printf("4\n");
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0x00});//panel setting
    EPD_2IN13B_V3_SendData(&(uint8_t){0x0f});//LUT from OTP，128x296
    EPD_2IN13B_V3_SendData(&(uint8_t){0x89});//Temperature sensor, boost and other related timing settings

    printf("5\n");
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0x61});//resolution setting
    EPD_2IN13B_V3_SendData (&(uint8_t){0x68});
    EPD_2IN13B_V3_SendData (&(uint8_t){0x00});
    EPD_2IN13B_V3_SendData (&(uint8_t){0xD4});

    printf("6\n");
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0X50});//VCOM AND DATA INTERVAL SETTING
    EPD_2IN13B_V3_SendData(&(uint8_t){0x77});//WBmode:VBDF 17|D7 VBDW 97 VBDB 57
                                 //WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7;
}

void EPD_2IN13B_V3_Clear(void)
{
    uint16_t Width = EPD_2IN13B_V3_WIDTH / 8;
    uint16_t Height = EPD_2IN13B_V3_HEIGHT;
    
    //send black data
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0x10});
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            EPD_2IN13B_V3_SendData(&(uint8_t){0xFF});
        }
    }

    //send red data
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0x13});
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            EPD_2IN13B_V3_SendData(&(uint8_t){0xFF});
        }
    }
    EPD_2IN13B_V3_TurnOnDisplay();
}

void EPD_2IN13B_V3_Display(const uint8_t* blackimage, const uint8_t* ryimage)
{
    uint16_t Width, Height;
    Width = EPD_2IN13B_V3_WIDTH / 8;
    Height = EPD_2IN13B_V3_HEIGHT;
    
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0x10});
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            EPD_2IN13B_V3_SendData(&(uint8_t){blackimage[i + j * Width]});
        }
    }
    
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0x13});
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            EPD_2IN13B_V3_SendData(&(uint8_t){ryimage[i + j * Width]});
        }
    }
    EPD_2IN13B_V3_TurnOnDisplay();
}

void EPD_2IN13B_V3_Sleep(void)
{
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0X50});
    EPD_2IN13B_V3_SendData(&(uint8_t){0xf7}); 

    EPD_2IN13B_V3_SendCommand(&(uint8_t){0X02}); //power off
    EPD_2IN13B_V3_ReadBusy();          //waiting for the electronic paper IC to release the idle signal
    EPD_2IN13B_V3_SendCommand(&(uint8_t){0X07}); //deep sleep
    EPD_2IN13B_V3_SendData(&(uint8_t){0xA5});
}



int main()
{
    sleep_ms(3000);
    stdio_init_all();

    // TODO init multiple pins at once
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    gpio_init(POT_OUT_PIN);
    gpio_set_dir(POT_OUT_PIN, GPIO_OUT);
    adc_init();
    adc_gpio_init(POT_IN_PIN);
    adc_select_input(POT_IN_PIN_ADC);

    gpio_init(LIGHT_OUT_PIN);
    gpio_set_dir(LIGHT_OUT_PIN, GPIO_OUT);
    gpio_init(LIGHT_IN_PIN);
    gpio_set_dir(LIGHT_IN_PIN, GPIO_IN);

    spi_init(spi_default, 1000*1000);
    gpio_set_function(SCRN_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(SCRN_TX, GPIO_FUNC_SPI);
    gpio_set_function(SCRN_CS, GPIO_FUNC_SPI);
    gpio_init(SCRN_PWR);
    gpio_set_dir(SCRN_PWR, GPIO_OUT);
    gpio_init(SCRN_DC);
    gpio_set_dir(SCRN_DC, GPIO_OUT);
    gpio_init(SCRN_RST);
    gpio_set_dir(SCRN_RST, GPIO_OUT);
    gpio_init(SCRN_BUSY);
    gpio_set_dir(SCRN_BUSY, GPIO_IN);

    gpio_put(POT_OUT_PIN, 1);
    gpio_put(LIGHT_OUT_PIN, 1);

    blink(2);

    printf("Starting scrn_test\n");
    scrn_test(1);

    while(true)
    {
        if(is_light()) single_blink(get_pot());
        else sleep_ms(DEFAULT_WAIT);
    }

    return 0;
}