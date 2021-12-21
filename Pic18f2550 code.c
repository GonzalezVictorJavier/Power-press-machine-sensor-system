#include <18F2550.h>
#device adc=10
#include <string.h>
#include <stdio.h>
#FUSES NOWDT                    //No Watch Dog Timer
#FUSES HS                       //Crystal osc >= 4mhz for PCM/PCH , 3mhz to 10 mhz for PCD
#FUSES NOPROTECT                //Code not protected from reading
#FUSES NOBROWNOUT               //No brownout reset
#FUSES BORV20                   //Brownout reset at 2.0V
#FUSES NOCPD                    //No EE protection
#FUSES NODEBUG                  //No Debug mode for ICD
#FUSES NOLVP                    //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES NOWRT                    //Program memory not write protected
#FUSES NOWRTD                   //Data EEPROM not write protected
#FUSES NOWRTC                   //configuration not registers write protected
#FUSES NOWRTB                   //Boot block not write protected
#FUSES NOEBTR                   //Memory not protected from table reads
#FUSES NOEBTRB                  //Boot block not protected from table reads
#FUSES NOCPB                    //No Boot Block code protection
#FUSES MCLR                     //Master Clear pin enabled
//#FUSES VREGEN                   //USB voltage regulator enabled

#use delay(clock=20000000)

//------------------------------------------------------------------------------
#define LCD_E    PIN_B4
#define LCD_RW   PIN_B3
#define LCD_RS   PIN_B2
#define LCD_DB4  PIN_B5
#define LCD_DB5  PIN_B6
#define LCD_DB6  PIN_B7
#define LCD_DB7  PIN_C6
#include "LCD_FLEXY.c"
//#USE STANDARD_IO(B)

#bit out = 0xF82.7
#bit autoset = 0xF81.0
#bit reset = 0xF81.1
#bit preset = 0xF82.0
#bit piezo = 0xF80.0
#bit descarga_clamp = 0xF80.1

#define adcconst 10.24
#define valor_filtro 102.4

int1 flagAutoset = 0;
float ref_sup = 99;
float ref_inf = 0;
float promedio;
float lectura;
char texto_lcd[24];
//------------------------------------------------------------------------------
void print_lcd(char * );
float lectura_adc(void );
void write_float_eeprom(long int , float ); 
float read_float_eeprom(long int ); 
void autoset(void );
//------------------------------------------------------------------------------
#INT_EXT
void int_ext_0()
{
   flagAutoset = 1;
   autoset();   
   output_b(input_b());
   clear_interrupt(INT_EXT);
   output_high(PIN_B0);
   flagAutoset = 0;
}
//------------------------------------------------------------------------------
#INT_EXT1
void int_ext_1()
{
   if(reset == 0)
   {
      out = 0;
   }
   output_b(input_b());
   clear_interrupt(INT_EXT1);
   output_high(PIN_B1);
}
//------------------------------------------------------------------------------
void main(void)
{
   descarga_clamp = 0;
   output_b(0b11111111);
   set_tris_a(0b00000001);
   set_tris_b(0b00000011);
   set_tris_c(0b00000001);
   setup_timer_1(T1_DISABLED);
   enable_interrupts(INT_EXT);
   Ext_Int_Edge(0,H_TO_L);
   clear_interrupt(INT_EXT);
   enable_interrupts(INT_EXT1);
   Ext_Int_Edge(1,H_TO_L);
   clear_interrupt(INT_EXT1);
   setup_adc(ADC_CLOCK_DIV_2);
   setup_adc_ports( AN0 );
   setup_spi(SPI_SS_DISABLED);
   setup_comparator(NC_NC_NC_NC);
   setup_vref(FALSE);
   setup_timer_0(RTCC_INTERNAL);
   enable_interrupts(global);
   out = 0;
   lcd_init();                                     
   set_adc_channel(0);
   lcd_gotoxy(1,1);  
   lcd_putc("HOLA");                                         
   delay_ms(2000);
   promedio = read_float_eeprom(sizeof(float) * 0);                                           
   ref_sup = read_float_eeprom(sizeof(float) * 1);
   ref_inf = read_float_eeprom(sizeof(float) * 2);
   lcd_gotoxy(1,1);  
   sprintf(texto_lcd, "Promedio:   \n%.2f", promedio);
   print_lcd(texto_lcd);
   lcd_putc("  ");                                         
   delay_ms(2000);
   lcd_gotoxy(1,1);  
   sprintf(texto_lcd, "Refsup:   \n%.2f", ref_sup);
   print_lcd(texto_lcd);
   lcd_putc("          ");                                         
   delay_ms(2000);
   lcd_gotoxy(1,1);  
   sprintf(texto_lcd, "Refinf:   \n%.2f", ref_inf);
   print_lcd(texto_lcd);
   lcd_putc("          ");                                         
   delay_ms(2000);
   do
   {
      lectura = lectura_adc();
      lcd_gotoxy(1,1);  
      sprintf(texto_lcd, "Medicion:   \n%.2f", lectura);
      print_lcd(texto_lcd);
      lcd_putc("  ");                                         
      if(lectura > ref_sup || lectura < ref_inf )
      {
         out = 1;
      }
      lectura = 0;
   }while(TRUE);
}
//------------------------------------------------------------------------------
void print_lcd(char * c)
{
   int incremento = 0;
   while(*(c + incremento) != '\0')
   {
      lcd_putc(* (c + incremento));
      incremento ++;
   }
}
//------------------------------------------------------------------------------
float lectura_adc()
{
   float lectura = 0;
   if(flagAutoset == 0)
   {
      enable_interrupts(INT_EXT);                                                  //Porque parece q el compilador las deshabilita
      enable_interrupts(INT_EXT1);
      enable_interrupts(global);
   }
   do
   {
   }while((float)read_adc() <= valor_filtro);                              
   delay_ms(100);
   lectura = read_adc();
   delay_ms(1);                                                                //Este es para matar los piquitos de despues de la medicion
   descarga_clamp = 1;
   delay_ms(100);                                                              //Para darle tiempo a q descargue el capa    descarga_clamp = 0;
   descarga_clamp = 0;
   delay_ms(50);
   (float)lectura = ((float)lectura /(float)adcconst);
   return lectura;
}
//------------------------------------------------------------------------------
void write_float_eeprom(long int n, float data)                                 
{
   int i;

   for (i = 0; i < 4; i++) 
      write_eeprom(i + n, *((int8*)&data + i) ) ; 
}
//------------------------------------------------------------------------------
float read_float_eeprom(long int n) 
{
   int i; 
   float data;

   for (i = 0; i < 4; i++) 
      *((int8*)&data + i) = read_eeprom(i + n);

   return(data); 
}
//------------------------------------------------------------------------------
void autoset()
{
   promedio = 0;
   lcd_gotoxy(1,1);  
   lcd_putc("Primer   \nmedicion");                                         
   lcd_putc("          ");                                        
   lectura = lectura_adc();
   promedio = promedio + lectura;
   lcd_gotoxy(1,1);  
   sprintf(texto_lcd, "Medicion:   \n%.2f", lectura);
   print_lcd(texto_lcd);
   lcd_putc("          ");                                         
   lectura = 0;
   delay_ms(2000);
         
   lcd_gotoxy(1,1);
   lcd_putc("Segunda   \nmedicion");
   lcd_putc("          ");                                         
   lectura = lectura_adc();
   promedio = promedio + lectura;
   lcd_gotoxy(1,1);  
   sprintf(texto_lcd, "Medicion:   \n%.2f", lectura);
   print_lcd(texto_lcd);
   lcd_putc("          ");                                                
   lectura = 0;
   delay_ms(2000);

   lcd_gotoxy(1,1);  
   lcd_putc("Tercer   \nmedicion");                                        
   lcd_putc("          ");                                         
   lectura = lectura_adc();
   promedio = promedio + lectura;
   lcd_gotoxy(1,1);  
   sprintf(texto_lcd, "Medicion:   \n%.2f", lectura);
   print_lcd(texto_lcd);
   lcd_putc("          ");                                         
   lectura = 0;
   delay_ms(2000);
   promedio = (float)promedio / (float)3;
   if(preset == 0)
   {
      ref_sup = ((float)promedio * (float)0.98);
      ref_inf = ((float)promedio * (float)1.02);
   }
   else
   {
      ref_sup = ((float)promedio * (float)0.98);
      ref_inf = ((float)promedio * (float)1.02);         
   }
   write_float_eeprom(sizeof(float) * 0 ,promedio );
   write_float_eeprom(sizeof(float) * 1 ,ref_sup );
   write_float_eeprom(sizeof(float) * 2 ,ref_inf );
   lcd_gotoxy(1,1);  
   sprintf(texto_lcd, "Promedio:   \n%.2f", promedio);
   print_lcd(texto_lcd);
   lcd_putc("          ");                                         
   delay_ms(1000);
   lcd_gotoxy(1,1);  
   sprintf(texto_lcd, "Refsup:   \n%.2f", ref_sup);
   print_lcd(texto_lcd);
   lcd_putc("          ");                                         
   delay_ms(1000);
   lcd_gotoxy(1,1);  
   sprintf(texto_lcd, "Refinf:   \n%.2f", ref_inf);
   print_lcd(texto_lcd);
   lcd_putc("          ");                                         
   delay_ms(1000);
   flagAutoset = 0;   
}

