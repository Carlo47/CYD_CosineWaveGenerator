/**
 * Program      CYD_CosineWaveGenerator.cpp
 * Author       2024-04-20 Charles Geiser (https://www.dodeka.ch)
 * 
 * Purpose      Shows how to use the builtin cosine wave generator of the ESP32
 * 
 * Board        ESP32-2432S028R Cheap Yellow Display. The output of DAC_2 (GPIO26) is routed through 
 *              an OPAMP to the speaker.
 *              Operation with modified OPAMP circuitry: R7 = R8 = 18k, R9 =33k
 *              Usable requency range: 15 .. 15'000 Hz Vpp 2.5V 
 *
 * Wiring       Oscilloscope on one of the SPKR pins and GND
 *
 * Remarks      The common output frequency of both DAC channels is given by the formula:
 * 
 *              freq = dig_clk_rtc_freq / (1 + RTC_CNTL_CK8M_DIV_SEL) * SENS_SAR_SW_FSTEP / 65536
 *              =================================================================================
 *              with
 *              dig_clk_rtc_freq = 8'000'000 Hz (assumed operating frequency, no need to know it exactly, see below)
 *              f0 = dig_clk_rtc_freq / 65536 = 8000000 / 65536 = 122.0703125
 *              we get
 *              freq = f0 * SENS_SAR_SW_FSTEP / (1 + RTC_CNTL_CK8M_DIV_SEL)
 *              freq = f0 * step / (1 + divi) 
 *              =============================
 *              divi = RTC_CNTL_CK8M_DIV_SEL = 0..7
 *              step = SENS_SAR_SW_FSTEP     = 1..65535 (0x0001 .. 0xffff)
 * 
 *              The resulting frequency range therefor is:
 *              fmin = f0 * 1 / (1 + 7) = 15.25 Hz
 *              fmax = f0 * 65535 / (1 + 0) = 7'999'877.9 Hz
 * 
 *              If we set SENS_SAR_SW_FSTEP = 1 and RTC_CNTL_CK8M_DIV_SEL = 0 
 *              we are able to measure f0 directly.
 *              This referenc frequency is ouput on startup of the program.
 *              We measure this frequency with a oscilloscope or a counter and 
 *              set it in the code or on the touchscreen to calibrate the cosine wave generator.
 *               
 * References   https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
 *              https://github.com/krzychb/dac-cosine
 *              https://www.esp32.com/viewtopic.php?t=10321
 *              https://www.dodeka.ch/CosineWaveGenerator.html
 *              https://github.com/rzeldent/platformio-espressif32-sunton/ (board definitions)
 *              https://www.youtube.com/watch?app=desktop&v=6JCLHIXXVus    (fixing the audio issues)
 */

#include <Arduino.h>
#include <SD.h>
#include "lgfx_ESP32_2432S028.h"
#include "CosineWaveGenerator.h"
#include "UiComponents.h"
#include "Wait.h"

using Action = void(&)(LGFX &lcd);
enum Rotation {PORTRAIT, LANDSCAPE};

LGFX lcd;
GFXfont myFont = fonts::DejaVu18;
//SPIClass sdcardSPI(VSPI); // uncomment line 56, 333 and 352 to take screenshot

constexpr int dig_clk_rtc_freq = 8000000.0;       // 8 MHz assumed operating frequency
constexpr double f0 = 132.5; //dig_clk_rtc_freq / 65536.0; // = 122.0703125 resulting reference frequency, change it to your measured frequency on startup
CosineWaveGenerator cwGen(f0);

extern void nop(LGFX &lcd);
extern void initDisplay(LGFX &lcd, uint8_t rotation=0, lgfx::v1::GFXfont *theFont=&myFont, Action greet=nop);
extern void initSDCard(SPIClass &spi);
extern void lcdInfo(LGFX &lcd);
extern void printSDCardInfo();
extern bool saveBmpToSD_16bit(LGFX &lcd, const char *filename);

class UiPanelTitle : public UiPanel
{
    public:
        UiPanelTitle(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) : 
            UiPanel(lcd, x, y, w, h, bgColor, hidden)
        {
            if (! _hidden) { show(); }
        }

        void show()
        {
            UiPanel::show();
            _lcd.setTextDatum(textdatum_t::middle_left);
            panelText(3, 12, "Cosine Wave Generator", TFT_MAROON, fonts::DejaVu18);
            panelText(20, 28, "f = f0 * step / (1 + divider)", TFT_BLACK, fonts::DejaVu12);
        }
    private:
};


class UiPanelCwGen : public UiPanel
{
    public:
        UiPanelCwGen(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) : 
            UiPanel(lcd, x, y, w, h, bgColor, hidden)
        {
            _frequency->setRange(15.0, 8000000.0);
            _f0->setRange(100.0, 150.0);
            _mode->setRange(0, 3);
            _divider->setRange(0, 7);
            _step->setRange(1, 65535);
            _tolerance->setRange(1, 999);
            if (! _hidden) { show(); }
        }

        void show()
        {
            UiPanel::show();
            for (int i = 0; i < _btns.size(); i++)
            {
                _btns.at(i)->draw();
            }
        }

        void handleKeys(int x, int y);
        std::vector<UiButton *> getButtons() { return _btns; }

    private:
      int d = 8;  // distance from the left panel side
      UiButton *_frequency = new UiButton(this, _x+d, _y+10,  200, 26, defaultTheme, "122.0703125", "f"); 
      UiButton *_f0        = new UiButton(this, _x+d, _y+50,  135, 26, "122.0703125", "f0");
      UiButton *_mode      = new UiButton(this, _x+d, _y+90,   30, 26, "2", "Mode 0..3");
      UiButton *_divider   = new UiButton(this, _x+d, _y+130,  30, 26, "0", "Divider 0..7");
      UiButton *_step      = new UiButton(this, _x+d, _y+170,  70, 26, "1", "Step 1..65535");
      UiButton *_tolerance = new UiButton(this, _x+d, _y+210,  70, 26, "10", "Tolerance o/oo");
      UiButton *_setMatch  = new UiLed(this, _x+d+44, _y+260,  12, TFT_GOLD, "Optimal match", true);

      std::vector<UiButton *> _btns = { _frequency, _f0, _mode, _divider, _step, _tolerance, _setMatch};
};

// Declare pointers to the panels and initialize them with nullptr
UiPanelTitle *panelTitle = nullptr;
UiPanelCwGen *panelCwGen = nullptr; 

// Declare the static class variable again in main
std::vector<UiPanel *> UiPanel::panels;

// Create they keypad hidden
UiKeypad keypad(lcd, 20,80, TFT_GOLD, true);
Wait waitUserInput(100);  // look for user input every 100 ms


void UiPanelCwGen::handleKeys(int x, int y)
{
  //freq = f0 * step / (1 + divi)
    int mode, step, divi, tol;
    double f, f0;

    for (int i = 0; i < _btns.size(); i++)
    {
        if (_btns.at(i)->touched(x, y)) 
        {
            log_i("Key pressed: %d", i);
            switch(i)
            {
                case 0: // frequency
                    _pKeypad->addValueField(_btns.at(i)); // Register the value field with the keypad
                    _pKeypad->show(); // show keypad 
                break;

                case 1: // f0
                    _pKeypad->addValueField(_btns.at(i)); // Register the value field with the keypad
                    _pKeypad->show(); // show keypad                    
                break;

                case 2: // mode
                    _pKeypad->addValueField(_btns.at(i)); // Register the value field with the keypad
                    _pKeypad->show(); // show keypad                    
                break;

                case 3: // divider
                    _pKeypad->addValueField(_btns.at(i)); // Register the value field with the keypad
                    _pKeypad->show(); // show keypad                   
                break;

                case 4: // step
                    _pKeypad->addValueField(_btns.at(i)); // Register the value field with the keypad
                    _pKeypad->show(); // show keypad                   
                break;

                case 5: // tolerance
                    _pKeypad->addValueField(_btns.at(i)); // Register the value field with the keypad
                    _pKeypad->show(); // show keypad                   
                break;

                case 6: // best/optimal match
                    UiLed * led = reinterpret_cast<UiLed *>(_btns.at(6));
                    led->toggle();
                    if (led->isOn())
                    {
                        led->setLabel("Optimal match");
                        panelCwGen->redrawPanels();
                    } 
                        
                    else
                    {
                        led->setLabel("Best match");
                        panelCwGen->redrawPanels();
                    }
                break; 
            }
            delay(500);
        }
    }
}

/**
 * Update frequency, divider and step when the frequency is entered with
 * the keypad. The function is registered as okCallback in the keypad. 
*/
void updateFrequency(UiButton *btn)
{
    int S[8];       // steps
    double F[8];    // resulting frequencies
    double D[8];    // delta abs(f_target - f)
    int mode, step, divi, tol, s, bestDivider, optimalDivisor;
    double f, ft, f0, q, minDelta;
    std::vector<UiButton *> btns = panelCwGen->getButtons();

    if (btn == btns.at(0)) // f
    {
        btns.at(0)->getValue(ft);  // get target frequency
        btns.at(1)->getValue(f0);  // f0
        q = ft / f0;
        btns.at(5)->getValue(tol);
        
        for (int d = 0; d < 8; d++)
        {
            S[d] = round(q * ( 1+ d));
            F[d] = f0 * S[d] / (1 + d);  // keep step s and resulting frequency f for all divisors d
            D[d] = abs(ft - F[d]);       // keep difference to target frequency
            Serial.printf("%d %2d %10.5f %10.5f\n", d, S[d], F[d], D[d]);
        }

        if (reinterpret_cast<UiLed *>(btns.at(6))->isOn())  // Optimal match
        {  // Find the smallest divider that results in a frequency within specified tolerance.
            log_i("Search optimal divider/step pair for given tolerance"); 
            minDelta = ft * tol/1000;
            bestDivider = 0;
            for (int d = 0; d < 8; d++)
            {
                if (D[d] <= minDelta) { bestDivider = d; d=8;}
            }  
        }
        else  // Best match. Find the best divider/step pair  
        {     // that best approximates the desired frequency  
            log_i("Search best divider/step pair for best approximation");   
            minDelta = D[7];
            bestDivider = 7;
            for (int d = 6; d > 0; d--)
            {
                if (D[d] <= minDelta) { bestDivider = d; minDelta = D[d]; }
            }
        }
        log_i("Divider=%d / step=%d", bestDivider, S[bestDivider]);
        f = f0 * S[bestDivider] / (1 + bestDivider);
        btns.at(3)->updateValue(bestDivider);
        btns.at(4)->updateValue(S[bestDivider]);
        btns.at(0)->updateValue(f);
        cwGen.setClockDivisor(bestDivider);
        cwGen.setFrequencyStep(S[bestDivider]);
    }

    if (btn == btns.at(1)) // f0
    {
        btns.at(1)->getValue(f0);
        btns.at(4)->getValue(step);
        btns.at(3)->getValue(divi);
        f = f0 * step / (1.0  + divi);
        btns.at(0)->updateValue(f);
        cwGen.setReferenceFrequency(f0);
    }

    if (btn == btns.at(2)) // mode
    {
        btns.at(2)->getValue(mode);
        log_i("Set mode to: %d", mode);
        cwGen.setMode(DAC_CHANNEL_2, (CWmode)mode);
    }

    if (btn == btns.at(3)) // divider
    {
        btns.at(3)->getValue(divi);
        btns.at(1)->getValue(f0);
        btns.at(4)->getValue(step);
        f = f0 * step / (1.0  + divi);
        btns.at(0)->updateValue(f);
        log_i("f=%.3g, f0=%.3g, divi=%d, step=%d", f, f0, divi, step);
        cwGen.setClockDivisor(divi);         
    }

    if (btn == btns.at(4)) // step
    {
        btns.at(4)->getValue(step);
        btns.at(1)->getValue(f0);
        btns.at(3)->getValue(divi);
        f = f0 * step / (1.0  + divi);
        btns.at(0)->updateValue(f);
        log_i("f=%.3g, f0=%.3g, divi=%d, step=%d", f, f0, divi, step);
        cwGen.setFrequencyStep(step);
    }

    if (btn == btns.at(5)) // tolerance
    {
        btns.at(5)->getValue(tol); 
        cwGen.setToleranceForBestMatch(tol);        
    }
}


/**
 * 👉 An empty white image is created when SD card 
 * and touch are both active. Why are the pixels
 * not output to the file? 
*/
void takeScreenshot()
{   
    static int count=0;
    char buf[64];
    snprintf(buf, sizeof(buf), "/SCREENSHOTS/screen%03d.bmp", count++);
    saveBmpToSD_16bit(lcd, buf);
    log_i("Screenshot saved: %s\n", buf);
}


void setup() 
{
  Serial.begin(115200);

  lcd.setBaseColor(DARKERGREY);
  initDisplay(lcd, Rotation::PORTRAIT, &myFont, lcdInfo);
  
  //initSDCard(sdcardSPI);      // Init SD card to take screenshots
  //printSDCardInfo();          // Print SD card details

  // Create the panels and showm them ( argument hidden is set to false)
  panelTitle = new UiPanelTitle(lcd, 0, 0,  lcd.width(), 35, TFT_GOLD,  false);
  panelCwGen = new UiPanelCwGen(lcd, 0, 35, lcd.width(), lcd.height(), TFT_MAROON,  false);
  panelCwGen->addKeypad(&keypad);         // add a keypad to enter numeric values 
  keypad.addOkCallback(updateFrequency);  // callback called when OK is tapped on the keyboard

  // Initialize the static class variable with all panels
  UiPanel::panels = { panelTitle, panelCwGen };

  cwGen.enable(DAC_CHANNEL_2);  // CYD uses DAC_CHANNEL_1 for CDS-LDR

  // Initialize the settings
  updateFrequency(panelCwGen->getButtons().at(1));
  updateFrequency(panelCwGen->getButtons().at(3));
  updateFrequency(panelCwGen->getButtons().at(4));

  //takeScreenshot(); // uncomment to take screenshot on startup
}


void loop() 
{
    int x, y;
    if (waitUserInput.isOver() && lcd.getTouch(&x, &y))
    {
        //log_i("Key pressed at %3d, %3d\n", x, y);
        if (!panelCwGen->isHidden()) panelCwGen->handleKeys(x, y);
        if (!keypad.isHidden())      keypad.handleKeys(x, y);
    }
}
