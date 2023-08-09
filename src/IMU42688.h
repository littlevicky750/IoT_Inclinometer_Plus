#ifndef IMU42688_h
#define IMU42688_h
#include <Arduino.h>
#include "WC_IMU.h"
#include "LEDFlash.h"
#include "SerialDebug.h"

#ifndef IMU_C
#define IMU_C 3
#endif

class IMU42688
{
private:
    /******************************************************************************************************
     * Parameter setting.
     *******************************************************************************************************/
    // For Debug
    const bool Skip_Warm_Up = false;              /** @brief True if want to run through the warm up procedure in 10 seconds @warning For debug only, never print in final version*/
    const bool Serial_Print_Update_Data = false; /** @brief Set true to print recieved IMU data. @warning For debug only, never print in final version*/

    // Warm-up Setting

    /**
     * @brief Temperature stability determination period ( seconds ).
     * @note Sensor temperature stability is used as the secondary reference for warm-up situation determination.
     * Therefore, peroid = 90 (1.5 minutes) is a good choice.
     */
    const int SteadyTempCalPeriod = 90;
    const float SteadyTempTH = 0.5;     /** @brief Temperature stability threshold ( C ).*/
    const float WarmUpTemperature = 40; /** @brief Default IMU warm-up complete temperature ( C ).*/

    // For calibration
    const float Cal_TH = 0.02; /** @brief Threshold value for identify the stability of IMU in calibration data collection.*/

    // For calibration UI
    const uint8_t Calibrate_Display_Length = 3; /** @brief Number of rows display in calibration method select UI.*/
    const uint8_t Cal_LED_Pri = 6;              /** @brief LED hint display priority for calibration ( 0 to 6 ) @note Make sure the code turn off the correct LED.*/

    /*****************************************************************************************************
     * IMU informations.
     ******************************************************************************************************/

    float AngleShow[3] = {0, 0, 0};          /** @brief Raw special filted angle in degree. < angle X , angle Y , angle Z > */
    float SensorTemperatureCollect[2] = {0}; /** @brief Raw IMU temperature. < T ( t ) , T ( t - dt ) >*/
    float MaxTemp = 0;                       /** @brief Detected maximum temperature within the temperature stability determination period.*/
    float minTemp = WarmUpTemperature;       /** @brief Detected minimum temperature within the temperature stability determination period.*/
    float StartTemperature;                  /** @brief Sensor intiail temperature*/
    int SteadyTempCalStart = 0;              /** @brief Timestamp when start the temperature stability determination period.*/
    int CopeFailed = 0;                      /** @brief Coped failed counter*/

    String String_rad(float degree);
    String String_mm(float degree);

    /*****************************************************************************************************
     * IMU parmeter load from flash memory.
     ******************************************************************************************************/

    float e[3] = {0.0}; /** @brief IMU error between user-set zero positions and raw data. < error X , error Y , error Z >*/
    float s[3] = {0.0}; /** @brief IMU data scale error. < scale X , scale Y , scale Z >*/
    float b[3] = {0.0}; /** @brief IMU bias between lab calibrated zero positions and raw data. < bias X , bias Y , bias Z >*/

    /********************************************************************************************************
     * Calibration Information.
     *********************************************************************************************************/
    bool Have_New_Data_for_Collect = false; /** @brief True if imu already update and the value havn't been collected by calibration.*/
    int StartCalG;                          /** @brief Gravity direction when the angle collection start. Used to identify the severe change on IMU's attitude.*/
    float StartCalA[3];                     /** @brief Angle value when the angle collection start. Used to identify IMU's stability.*/
    float CalibrateCollect[6][10] = {0.0};  /** @brief \b FullCalibrate_Z() angle measurement result buffer.*/
    float SumCalA[3];                       /** @brief \b CollectCalData() collected angle summation.*/
    float FullCalAngle[6] = {0.0};          /** @brief Result angle in corresponding gravity direction.*/

    float Avg_in_2StDev(float *Angle, bool *Count, int length);
    void CollectCalData();
    void QuickCalibrate();
    void FullCalibrate();
    void FullCalibrate_Z();
    void ClearZeros();
    void SetCalLED(uint8_t Flash);

public:
    /********************************************************************************************************
     * Remember to set these values in setup()
     * Chance to cause the panic / crash / ... if not setting these pointer.
     ********************************************************************************************************/
    int *fWarmUpTime; /*Pointer to the time-off auto-sleep clock*/
    LEDFlash *pLED;   /*Pointer to SLED for hint light*/

    /********************************************************************************************************
     * IMU Information
     ********************************************************************************************************/
    /**
     * @brief Indicate current IMU update situations.
     * @note 0 - Update success.
     * @note 1 - IMU not warm up.
     * @note 2 - Recieve data formatting error.
     * @note 3 - Recieve angle value located outside the threshold.
     * @note >4 - Times that IMU coped failed. Usually because Serial1 not available.
     * */
    uint8_t ErrorCode = Err_IMU_Not_Warm_Up;
    const uint8_t IMU_Update_Success = 0;                   /** Current imu Update situation indicator.*/
    const uint8_t Err_IMU_Not_Warm_Up = 1;                  /** Current imu Update situation indicator.*/
    const uint8_t Err_IMU_Receive_Data_Error = 2;           /** Current imu Update situation indicator.*/
    const uint8_t Err_IMU_Data_StDev_Outside_TrustHold = 3; /** Current imu Update situation indicator.*/
    const uint8_t Err_IMU_Cope_Failed = 4;                  /** Current imu Update situation indicator.*/

    /**
     * @brief Current Gravity direction
     * @note 0 : +X direction @note 1 : +Y direction @note 2 : +Z direction
     * @note 3 : -X direction @note 4 : -Y direction @note 5 : -Z direction
     */
    uint8_t Gravity = 2;

    /**
     * @brief Gravity Direction for interface rotation
     * @note 0 : +X direction @note 1 : +Y direction @note 3 : -X direction @note 4 : -Y direction
     */
    uint8_t GravityPrevious = 1;

    float Angle[3] = {0, 0, 0};        /** @brief Raw real-time angle in degree. < angle X , angle Y , angle Z >*/
    float AngleCal[3] = {0, 0, 0};     /** @brief Calibrated real-time angle in degree. < angle X , angle Y , angle Z >*/
    float AngleCalShow[3] = {0, 0, 0}; /** @brief Calibrated special filted angle in degree (for display only). < angle X , angle Y , angle Z >*/
    float SensorTemperature;           /** @brief Filted sensor temperature ( C ).*/
    uint8_t unit = 0;                  /** @brief Current interface levelness display unit. @note 0 : degree @note  1 : mm/m @note 2 : radian */
    uint8_t fWarmUp;                   /** @brief IMU warm-up % (0 to 100).*/
    String Cal_Info_From_Flash = "";   /** @brief IMU calibrated parameter load from flash memory.*/

    /**********************************************************************************************************
     * IMU Regular Function
     **********************************************************************************************************/

    void Initialize(byte Rx /*(-1)*/, byte Tx /*(-1)*/);
    uint8_t Update();
    String String_degree(float degree);
    String String_now_unit(float degree);
    void SetUnit(String Info);

    /**********************************************************************************************************
     * Calibrate function
     **********************************************************************************************************/

    /*********************************************************************************************************
     * Calibration UI ( user interface ) relative,
     * Put these parameters and function here in order to make the UI code look clear.
     * If the complex code is acceptable in the UI code, than it is better to rewrite these part of code in the UI code file.
     **********************************************************************************************************/

    /**
     * @brief Indicate the user calibration status.
     * @note \b CalibrateCheck value indicate that the user is currently in the state listed below :
     * @note -1 : Not in the calibration procedure or in the calibration select menu.
     * @note 0 : In the confirm page.
     * @note 1 : Pass the confirm page and start collecting the data.
     * @note 2 : Calibration done.
     * @note --------------------------------------------------------------------
     * @note The calibration procedure may be different in different calibration method selections.
     * For example, if the calibration require 3 times measure result, the procedure may appear like
     * @note -1 -> 0 -> 1 -> 0 -> 1 -> 0 -> 1 -> 2
     * @note If user select "Back" or "No" in the confirm page, the procedure will turn back to -1.
     */
    int CalibrateCheck = -1;

    /**
     * @brief Calibration method selection cursor.
     * @note Calibration method select menu arrange as below :
     * @note 0 : Back to page ---.
     * @note 1 : Quick calibration.
     * @note 2 : Reset user-set zero position error.
     * @note 3 : Full calibrate.
     * @note 4 : Full calibration for Z axis
     */
    uint8_t Cursor = 0;
    uint8_t CursorStart = 0; /** @brief Method selection list start displaying flag. @see \b Cursor */
    bool YesNo = false;      /** @brief Yes No Cursor on action confirm page.*/

    const uint8_t CalAvgNum = 50;           /** @brief The required number of raw angle data for generating one angle result.*/
    uint8_t CalibrateCount = 0;             /** @brief The number of collected raw angle data for generating the angle result.*/
    uint8_t CalibrateCollectCount[6] = {0}; /** @brief The number of angle measurement result on corresponding gravity direction.*/
    bool FullCalComplete[6] = {false};      /** @brief True if number of measurement result is enough in the corresponding direction.*/

    void CalibrateSelect(byte Do);
    void CalStop();
    void Calibrate();

    int FullCalStep = 0;

    /**
     * @brief True if allow getting into full calibration function.
     * @attention Used in DigitalSpiritLevel series only.
     * @attention For DigitalStraightEdge series, ExpertMode should always be false.
     */
    bool ExpertMode = false;

    /***********************************************************
     * Function not used currently.
     ***********************************************************/

    float getHorizontal();
    float getVertical();
    float getHorizontalFilt();
    float getVerticalFilt();
};

#endif