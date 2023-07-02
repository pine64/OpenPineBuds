#ifndef _GLP_COMMON_H_
#define _GLP_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup GLP Glucose Profile
 * @ingroup PROFILE
 * @brief Glucose Profile
 *
 * The GLP module is the responsible block for implementing the Glucose Profile
 * functionalities in the BLE Host.
 *
 * The Glucose Profile defines the functionality required in a device that allows
 * the user (Collector device) to configure and recover Glucose measurements from
 * a Glucose device.
 *****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include "prf_types.h"
#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

/// Glucose measurement packet max length
#define GLP_MEAS_MAX_LEN                (17)
/// Glucose measurement context packet max length
#define GLP_MEAS_CTX_MAX_LEN            (17)
/// Record Access Control Point packet max length
#define GLP_REC_ACCESS_CTRL_MAX_LEN     (21)


/// Glucose Service Error Code
enum glp_error_code
{
    /// RACP Procedure already in progress
    GLP_ERR_PROC_ALREADY_IN_PROGRESS  = (0x80),
    /// Client Characteristic Configuration Descriptor Improperly Configured
    GLP_ERR_IMPROPER_CLI_CHAR_CFG     = (0x81),
};

/// Glucose Measurement Flags field bit values
enum glp_meas_flag
{
    /// Time Offset Present Bit
    /// Time Offset Present
    GLP_MEAS_TIME_OFF_PRES = (1 << 0),
    /// Time Offset Not Present
    GLP_MEAS_TIME_OFF_NOT_PRES = (0 << 0),

    /// Glucose Concentration, Type and Sample Location Present Bit
    /// Glucose Concentration, Type and Sample Location Present
    GLP_MEAS_GL_CTR_TYPE_AND_SPL_LOC_PRES = (1 << 1),
    /// Glucose Concentration, Type and Sample Location Not Present
    GLP_MEAS_GL_CTR_TYPE_AND_SPL_LOC_NOT_PRES = (0 << 1),

    /// Glucose Concentration Units (kg/L, mol/L)
    GLP_MEAS_GL_CTR_UNITS_KG_L = (0 << 2),
    GLP_MEAS_GL_CTR_UNITS_MOL_L = (1 << 2),

    /// Sensor Status Annunciation Present Bit
    /// Sensor Status Annunciation Present
    GLP_MEAS_SENS_STAT_ANNUN_PRES = (1 << 3),
    /// Sensor Status Annunciation Not Present
    GLP_MEAS_SENS_STAT_ANNUN_NOT_PRES = (0 << 3),

    /// Context Information Follow Bit
    /// Context Information Follows
    GLP_MEAS_CTX_INF_FOLW = (1 << 4),
    /// Context Information Not Follows
    GLP_MEAS_CTX_INF_NOT_FOLW = (0 << 4),
};

/// Glucose Measurement Context Flags field bit values
enum glp_meas_ctx_flag
{
    /// Carbohydrate ID And Carbohydrate Present Bit
    /// Carbohydrate ID And Carbohydrate Present
    GLP_CTX_CRBH_ID_AND_CRBH_PRES = (1 << 0),
    /// Carbohydrate ID And Carbohydrate Not Present
    GLP_CTX_CRBH_ID_AND_CRBH_NOT_PRES = (0 << 0),

    /// Meal Present Bit
    /// Meal Present
    GLP_CTX_MEAL_PRES = (1 << 1),
    /// Meal Not Present
    GLP_CTX_MEAL_NOT_PRES = (0 << 1),

    /// Tester-Health Present Bit
    /// Tester-Health Present
    GLP_CTX_TESTER_HEALTH_PRES = (1 << 2),
    /// Tester-Health Not Present
    GLP_CTX_TESTER_HEALTH_NOT_PRES = (0 << 2),

    /// Exercise Duration And Exercise Intensity Present Bit
    /// Exercise Duration And Exercise Intensity Present
    GLP_CTX_EXE_DUR_AND_EXE_INTENS_PRES = (1 << 3),
    /// Exercise Duration And Exercise Intensity Not Present
    GLP_CTX_EXE_DUR_AND_EXE_INTENS_NOT_PRES = (0 << 3),

    /// Medication ID And Medication Present Bit
    /// Medication ID And Medication Present
    GLP_CTX_MEDIC_ID_AND_MEDIC_PRES = (1 << 4),
    /// Medication ID And Medication Not Present
    GLP_CTX_MEDIC_ID_AND_MEDIC_NOT_PRES = (0 << 4),

    /// Medication Value Units (kilograms, liters)
    GLP_CTX_MEDIC_VAL_UNITS_KG = (0 << 5),
    GLP_CTX_MEDIC_VAL_UNITS_L = (1 << 5),

    /// HbA1c Present Bit
    /// HbA1c Present
    GLP_CTX_HBA1C_PRES = (1 << 6),
    /// HbA1c Not Present
    GLP_CTX_HBA1C_NOT_PRES = (0 << 6),

    /// Extended Flags Present Bit
    /// Extended Flags Present
    GLP_CTX_EXTD_F_PRES = (1 << 7),
    /// Extended Flags Not Present
    GLP_CTX_EXTD_F_NOT_PRES = (0 << 7),
};

/// Glucose Service Feature Flags field bit values
enum glp_srv_feature_flag
{
    /// Low Battery Detection During Measurement Support Bit
    /// Low Battery Detection During Measurement Supported
    GLP_FET_LOW_BAT_DET_DUR_MEAS_SUPP = (1 << 0),
    /// Low Battery Detection During Measurement Not Supported
    GLP_FET_LOW_BAT_DET_DUR_MEAS_NOT_SUPP = (0 << 0),

    /// Sensor Malfunction Detection Support Bit
    /// Sensor Malfunction Detection Supported
    GLP_FET_SENS_MFNC_DET_SUPP = (1 << 1),
    /// Sensor Malfunction Detection Not Supported
    GLP_FET_SENS_MFNC_DET_NOT_SUPP = (0 << 1),

    /// Sensor Sample Size Support Bit
    /// Sensor Sample Size Supported
    GLP_FET_SENS_SPL_SIZE_SUPP = (1 << 2),
    /// Sensor Sample Size Not Supported
    GLP_FET_SENS_SPL_SIZE_NOT_SUPP = (0 << 2),

    /// Sensor Strip Insertion Error Detection Support Bit
    /// Sensor Strip Insertion Error Detection Supported
    GLP_FET_SENS_STRIP_INSERT_ERR_DET_SUPP = (1 << 3),
    /// Sensor Strip Insertion Error Detection Not Supported
    GLP_FET_SENS_STRIP_INSERT_ERR_DET_NOT_SUPP = (0 << 3),

    /// Sensor Strip Type Error Detection Support Bit
    /// Sensor Strip Type Error Detection Supported
    GLP_FET_SENS_STRIP_TYPE_ERR_DET_SUPP = (1 << 4),
    /// Sensor Strip Type Error Detection Not Supported
    GLP_FET_SENS_STRIP_TYPE_ERR_DET_NOT_SUPP = (0 << 4),

    /// Sensor Result High-Low Detection Support Bit
    /// Sensor Result High-Low Detection Supported
    GLP_FET_SENS_RES_HIGH_LOW_DET_SUPP = (1 << 5),
    /// Sensor Result High-Low Detection Not Supported
    GLP_FET_SENS_RES_HIGH_LOW_DET_NOT_SUPP = (0 << 5),

    /// Sensor Temperature High-Low Detection Support Bit
    /// Sensor Temperature High-Low Detection Supported
    GLP_FET_SENS_TEMP_HIGH_LOW_DET_SUPP = (1 << 6),
    /// Sensor Temperature High-Low Detection Not Supported
    GLP_FET_SENS_TEMP_HIGH_LOW_DET_NOT_SUPP = (0 << 6),

    /// Sensor Read Interrupt Detection Support Bit
    /// Sensor Read Interrupt Detection Supported
    GLP_FET_SENS_RD_INT_DET_SUPP = (1 << 7),
    /// Sensor Read Interrupt Detection Not Supported
    GLP_FET_SENS_RD_INT_DET_NOT_SUPP = (0 << 7),

    /// General Device Fault Support Bit
    /// General Device Fault Supported
    GLP_FET_GEN_DEV_FLT_SUPP = (1 << 8),
    /// General Device Fault Not Supported
    GLP_FET_GEN_DEV_FLT_NOT_SUPP = (0 << 8),

    /// Time Fault Support Bit
    /// Time Fault Supported
    GLP_FET_TIME_FLT_SUPP = (1 << 9),
    /// Time Fault Not Supported
    GLP_FET_TIME_FLT_NOT_SUPP = (0 << 9),

    /// Multiple Bond Support Bit
    /// Multiple Bond Supported
    GLP_FET_MUL_BOND_SUPP = (1 << 10),
    /// Multiple Bond Not Supported
    GLP_FET_MUL_BOND_NOT_SUPP = (0 << 10),
};


/// Glucose measurement type
enum glp_meas_type
{
    /// Capillary Whole blood
    GLP_TYPE_CAPILLARY_WHOLE_BLOOD = (1),

    /// Capillary Plasma
    GLP_TYPE_CAPILLARY_PLASMA = (2),

    /// Venous Whole blood
    GLP_TYPE_VENOUS_WHOLE_BLOOD = (3),

    /// Venous Plasma
    GLP_TYPE_VENOUS_PLASMA = (4),

    /// Arterial Whole blood
    GLP_TYPE_ARTERIAL_WHOLE_BLOOD = (5),

    /// Arterial Plasma
    GLP_TYPE_ARTERIAL_PLASMA = (6),

    /// Undetermined Whole blood
    GLP_TYPE_UNDETERMINED_WHOLE_BLOOD = (7),

    /// Undetermined Plasma
    GLP_TYPE_UNDETERMINED_PLASMA = (8),

    /// Interstitial Fluid (ISF)
    GLP_TYPE_INTERSTITIAL_FLUID = (9),

    /// Control Solution
    GLP_TYPE_CTRL_SOLUTION = (10),
};


/// Glucose measurement Sample Location
enum glp_meas_loc
{
    /// Finger
    GLP_LOC_FINGER = (1),

    /// Alternate Site Test (AST)
    GLP_LOC_ALT_SITE_TEST = (2),

    /// Earlobe
    GLP_LOC_EARLOBE = (3),

    /// Control solution
    GLP_LOC_CTRL_SOLUTION = (4),

    /// Sample Location value not available
    GLP_LOC_SPL_LOC_VAL_NOT_AVA = (15),
};

/// Glucose measurement Sensor Status Annunciation
enum glp_meas_state
{
   /// Device battery low at time of measurement
   GLP_MEAS_STATE_DEV_BAT_LOW = (1 << 0),

   /// Sensor malfunction or faulting at time of measurement
   GLP_MEAS_STATE_SENS_MFNC_OR_FLTING = (1 << 1),

   /// Sample size for blood or control solution insufficient at time of measurement
   GLP_MEAS_STATE_SPL_SIZE_INSUFF = (1 << 2),

   /// Strip insertion error
   GLP_MEAS_STATE_STRIP_INSERT_ERR = (1 << 3),

   /// Strip type incorrect for device
   GLP_MEAS_STATE_STRIP_TYPE_INCOR_FOR_DEV = (1 << 4),

   /// Sensor result higher than the device can process
   GLP_MEAS_STATE_SENS_RES_HIGHER = (1 << 5),

   /// Sensor result lower than the device can process
   GLP_MEAS_STATE_SENS_RES_LOWER = (1 << 6),

   /// Sensor temperature too high for valid test/result at time of measurement
   GLP_MEAS_STATE_SENS_TEMP_TOO_HIGH = (1 << 7),

   /// Sensor temperature too low for valid test/result at time of measurement
   GLP_MEAS_STATE_SENS_TEMP_TOO_LOW = (1 << 8),

   /// Sensor read interrupted because strip was pulled too soon at time of measurement
   GLP_MEAS_STATE_SENS_RD_INTED = (1 << 9),

   /// General device fault has occurred in the sensor
   GLP_MEAS_STATE_GEN_DEV_FLT = (1 << 10),

   /// Time fault has occurred in the sensor and time may be inaccurate
   GLP_MEAS_STATE_TIME_FLT = (1 << 11),
};


/// Glucose measurement context Carbohydrate ID
enum glp_meas_ctx_carbo
{
    /// Breakfast
    GLP_CID_BREAKFAST = (1),

    /// Lunch
    GLP_CID_LUNCH = (2),

    /// Dinner
    GLP_CID_DINNER = (3),

    /// Snack
    GLP_CID_SNACK = (4),

    /// Drink
    GLP_CID_DRINK = (5),

    /// Supper
    GLP_CID_SUPPER = (6),

    /// Brunch
    GLP_CID_BRUNCH = (7),
};

/// Glucose measurement context Meal
enum glp_meas_ctx_meal
{
    /// Preprandial (before meal)
    GLP_MEAL_PREPRANDIAL = (1),

    /// Postprandial (after meal)
    GLP_MEAL_POSTPRANDIAL = (2),

    /// Fasting
    GLP_MEAL_FASTING = (3),

    /// Casual (snacks, drinks, etc.)
    GLP_MEAL_CASUAL = (4),

    /// Bedtime
    GLP_MEAL_BEDTIME = (5),
};

/// Glucose measurement context tester type
enum glp_meas_ctx_tester_type
{
    /// Self
    GLP_TESTER_SELF = (1),

    /// Health Care Professional
    GLP_TESTER_HEALTH_CARE_PROFESSIONAL = (2),

    /// Lab test
    GLP_TESTER_LAB_TEST = (3),

    /// Tester value not available
    GLP_TESTER_TESTER_VAL_NOT_AVA = (15),
};

/// Glucose measurement context personal health feeling
enum glp_meas_ctx_feeling
{
    /// Minor health issues
    GLP_HEALTH_MINOR_HEALTH_ISSUES = (1),

    /// Major health issues
    GLP_HEALTH_MAJOR_HEALTH_ISSUES = (2),

    /// During menses
    GLP_HEALTH_DUR_MENSES = (3),

    /// Under stress
    GLP_HEALTH_UNDER_STRESS = (4),

    /// No health issues
    GLP_HEALTH_NO_HEALTH_ISSUES = (5),

    /// Health value not available
    GLP_HEALTH_HEALTH_VAL_NOT_AVA = (15),
};

/// Glucose measurement context medication identification
enum glp_meas_ctx_med_id
{
    /// Rapid acting insulin
    GLP_MEDID_RAPID_ACTING_INSULIN = (1),

    /// Short acting insulin
    GLP_MEDID_SHORT_ACTING_INSULIN = (2),

    /// Intermediate acting insulin
    GLP_MEDID_INTER_ACTING_INSULIN = (3),

    /// Long acting insulin
    GLP_MEDID_LONG_ACTING_INSULIN = (4),

    /// Pre-mixed insulin
    GLP_MEDID_PRE_MIXED_INSULIN = (5),
};


/// Glucose access control OP Code
enum glp_racp_op_code
{
    /// Report stored records (Operator: Value from Operator Table)
    GLP_REQ_REP_STRD_RECS = (1),

    /// Delete stored records (Operator: Value from Operator Table)
    GLP_REQ_DEL_STRD_RECS = (2),

    /// Abort operation (Operator: Null 'value of 0x00 from Operator Table')
    GLP_REQ_ABORT_OP = (3),

    /// Report number of stored records (Operator: Value from Operator Table)
    GLP_REQ_REP_NUM_OF_STRD_RECS = (4),

    /// Number of stored records response (Operator: Null 'value of 0x00 from Operator Table')
    GLP_REQ_NUM_OF_STRD_RECS_RSP = (5),

    /// Response Code (Operator: Null 'value of 0x00 from Operator Table')
    GLP_REQ_RSP_CODE = (6),
};

/// Glucose access control Operator
enum glp_racp_operator
{
    /// All records
    GLP_OP_ALL_RECS = (1),

    /// Less than or equal to
    GLP_OP_LT_OR_EQ = (2),

    /// Greater than or equal to
    GLP_OP_GT_OR_EQ = (3),

    /// Within range of (inclusive)
    GLP_OP_WITHIN_RANGE_OF = (4),

    /// First record(i.e. oldest record)
    GLP_OP_FIRST_REC = (5),

    /// Last record (i.e. most recent record)
    GLP_OP_LAST_REC = (6),
};

/// Glucose access control response code
enum glp_racp_status
{
    /// Success
    GLP_RSP_SUCCESS = (1),

    /// Op Code not supported
    GLP_RSP_OP_CODE_NOT_SUP = (2),

    /// Invalid Operator
    GLP_RSP_INVALID_OPERATOR = (3),

    /// Operator not supported
    GLP_RSP_OPERATOR_NOT_SUP = (4),

    /// Invalid Operand
    GLP_RSP_INVALID_OPERAND = (5),

    /// No records found
    GLP_RSP_NO_RECS_FOUND = (6),

    /// Abort unsuccessful
    GLP_RSP_ABORT_UNSUCCESSFUL = (7),

    /// Procedure not completed
    GLP_RSP_PROCEDURE_NOT_COMPLETED = (8),

    /// Operand not supported
    GLP_RSP_OPERAND_NOT_SUP = (9),
};


/// record access control filter type
enum glp_racp_filter
{
    /// Filter using Sequence number
    GLP_FILTER_SEQ_NUMBER = (1),
    /// Filter using Facing time
    GLP_FILTER_USER_FACING_TIME = (2),
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */





/// Glucose measurement
struct glp_meas
{
    /// Base Time
    struct prf_date_time base_time;
    /// Time Offset
    int16_t time_offset;
    /// Glucose Concentration - units of kg/L or mol/L
    prf_sfloat concentration;
    ///Sensor Status Annunciation
    uint16_t status;
    /// Type
    uint8_t type;
    /// Sample Location
    uint8_t location;
    /// Flags
    uint8_t flags;
};

/// Glucose measurement context
struct glp_meas_ctx
{
    /// Carbohydrate - units of kilograms
    prf_sfloat carbo_val;
    /// Exercise Duration
    uint16_t exercise_dur;
    /// Medication value (units of kilograms or liters)
    prf_sfloat med_val;
    /// HbA1c value
    prf_sfloat hba1c_val;

    /// Carbohydrate ID
    uint8_t carbo_id;
    /// Meal
    uint8_t meal;
    /// Tester
    uint8_t tester;
    /// Health
    uint8_t health;
    /// Exercise Intensity
    uint8_t exercise_intens;
    /// Medication ID
    uint8_t med_id;

    /// Flag
    uint8_t flags;
    /// Extended Flags
    uint8_t ext_flags;
};


/// Record access control point operation filter
struct glp_filter
{
    /// function operator
    uint8_t operator;

    /// filter type
    uint8_t filter_type;

    /// filter union
    union
    {
        /// Sequence number filtering (filter_type = GLP_FILTER_SEQ_NUMBER)
        struct
        {
            /// Min sequence number
            uint16_t min;
            /// Max sequence number
            uint16_t max;
        }seq_num;

        /// User facing time filtering (filter_type = GLP_FILTER_USER_FACING_TIME)
        struct
        {
            /// Min base time
            struct prf_date_time facetime_min;

            /// Max base time
            struct prf_date_time facetime_max;
        } time;
    } val;
};



/// Record access control point request
struct glp_racp_req
{
    /// operation code
    uint8_t op_code;

    /// operation filter
    struct glp_filter filter;
};



/// Record access control point response
struct glp_racp_rsp
{
    /// operation code
    uint8_t op_code;

    /// operator (ignored, always set to null)
    uint8_t operator;

    /// response operand
    union
    {
        /// Number of record (if op_code = GLP_REQ_NUM_OF_STRD_RECS_RSP)
        uint16_t num_of_record;

        /// Command Status (if op_code = GLP_REQ_RSP_CODE)
        struct
        {
            /// Request Op Code
            uint8_t op_code_req;
            /// Response Code status
            uint8_t status;
        } rsp;
    } operand;
};



/// @} glp_common

#endif /* _GLP_COMMON_H_ */
