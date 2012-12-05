/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/** Definition of subsystem numbers, to assist in creating a unique identifier
 * for each U/K call.
 *
 * @see _mali_uk_functions */
typedef enum
{
    _MALI_UK_CORE_SUBSYSTEM,      /**< Core Group of U/K calls */
    _MALI_UK_MEMORY_SUBSYSTEM,    /**< Memory Group of U/K calls */
    _MALI_UK_PP_SUBSYSTEM,        /**< Fragment Processor Group of U/K calls */
    _MALI_UK_GP_SUBSYSTEM,        /**< Vertex Processor Group of U/K calls */
	_MALI_UK_PROFILING_SUBSYSTEM, /**< Profiling Group of U/K calls */
    _MALI_UK_PMM_SUBSYSTEM,       /**< Power Management Module Group of U/K calls */
} _mali_uk_subsystem_t;

/** Within a function group each function has its unique sequence number
 * to assist in creating a unique identifier for each U/K call.
 *
 * An ordered pair of numbers selected from
 * ( \ref _mali_uk_subsystem_t,\ref  _mali_uk_functions) will uniquely identify the
 * U/K call across all groups of functions, and all functions. */
typedef enum
{
	/** Core functions */

    _MALI_UK_OPEN                    = 0, /**< _mali_ukk_open() */
    _MALI_UK_CLOSE,                       /**< _mali_ukk_close() */
    _MALI_UK_GET_SYSTEM_INFO_SIZE,        /**< _mali_ukk_get_system_info_size() */
    _MALI_UK_GET_SYSTEM_INFO,             /**< _mali_ukk_get_system_info() */
    _MALI_UK_WAIT_FOR_NOTIFICATION,       /**< _mali_ukk_wait_for_notification() */
    _MALI_UK_GET_API_VERSION,             /**< _mali_ukk_get_api_version() */

	/** Memory functions */

    _MALI_UK_INIT_MEM                = 0, /**< _mali_ukk_init_mem() */
    _MALI_UK_TERM_MEM,                    /**< _mali_ukk_term_mem() */
    _MALI_UK_GET_BIG_BLOCK,               /**< _mali_ukk_get_big_block() */
    _MALI_UK_FREE_BIG_BLOCK,              /**< _mali_ukk_free_big_block() */
    _MALI_UK_MAP_MEM,                     /**< _mali_ukk_mem_mmap() */
    _MALI_UK_UNMAP_MEM,                   /**< _mali_ukk_mem_munmap() */
    _MALI_UK_QUERY_MMU_PAGE_TABLE_DUMP_SIZE, /**< _mali_ukk_mem_get_mmu_page_table_dump_size() */
    _MALI_UK_DUMP_MMU_PAGE_TABLE,         /**< _mali_ukk_mem_dump_mmu_page_table() */
    _MALI_UK_ATTACH_UMP_MEM,             /**< _mali_ukk_attach_ump_mem() */
    _MALI_UK_RELEASE_UMP_MEM,           /**< _mali_ukk_release_ump_mem() */
    _MALI_UK_MAP_EXT_MEM,                 /**< _mali_uku_map_external_mem() */
    _MALI_UK_UNMAP_EXT_MEM,               /**< _mali_uku_unmap_external_mem() */
    _MALI_UK_VA_TO_MALI_PA,               /**< _mali_uku_va_to_mali_pa() */

    /** Common functions for each core */

    _MALI_UK_START_JOB           = 0,     /**< Start a Fragment/Vertex Processor Job on a core */
	_MALI_UK_ABORT_JOB,                   /**< Abort a job */
    _MALI_UK_GET_NUMBER_OF_CORES_R2P1,         /**< Get the number of Fragment/Vertex Processor cores */
    _MALI_UK_GET_CORE_VERSION_R2P1,            /**< Get the Fragment/Vertex Processor version compatible with all cores */

    _MALI_UK_GET_NUMBER_OF_CORES_R3P0 = 1,
    _MALI_UK_GET_CORE_VERSION_R3P0 = 2,

    /** Fragment Processor Functions  */

    _MALI_UK_PP_START_JOB            = _MALI_UK_START_JOB,            /**< _mali_ukk_pp_start_job() */
    _MALI_UK_PP_ABORT_JOB            = _MALI_UK_ABORT_JOB,            /**< _mali_ukk_pp_abort_job() */
    _MALI_UK_GET_PP_NUMBER_OF_CORES_R2P1  = _MALI_UK_GET_NUMBER_OF_CORES_R2P1,  /**< _mali_ukk_get_pp_number_of_cores() */
    _MALI_UK_GET_PP_CORE_VERSION_R2P1     = _MALI_UK_GET_CORE_VERSION_R2P1,     /**< _mali_ukk_get_pp_core_version() */

    _MALI_UK_GET_PP_NUMBER_OF_CORES_R3P0  = _MALI_UK_GET_NUMBER_OF_CORES_R3P0,
    _MALI_UK_GET_PP_CORE_VERSION_R3P0     = _MALI_UK_GET_CORE_VERSION_R3P0,

    /** Vertex Processor Functions  */

    _MALI_UK_GP_START_JOB            = _MALI_UK_START_JOB,            /**< _mali_ukk_gp_start_job() */
    _MALI_UK_GP_ABORT_JOB            = _MALI_UK_ABORT_JOB,            /**< _mali_ukk_gp_abort_job() */
    _MALI_UK_GET_GP_NUMBER_OF_CORES_R2P1  = _MALI_UK_GET_NUMBER_OF_CORES_R2P1,  /**< _mali_ukk_get_gp_number_of_cores() */
    _MALI_UK_GET_GP_CORE_VERSION_R2P1     = _MALI_UK_GET_CORE_VERSION_R2P1,     /**< _mali_ukk_get_gp_core_version() */

    _MALI_UK_GET_GP_NUMBER_OF_CORES_R3P0  = _MALI_UK_GET_NUMBER_OF_CORES_R3P0,
    _MALI_UK_GET_GP_CORE_VERSION_R3P0     = _MALI_UK_GET_CORE_VERSION_R3P0,


	/** Profiling functions */

	_MALI_UK_PROFILING_START         = 0, /**< __mali_uku_profiling_start() */
	_MALI_UK_PROFILING_ADD_EVENT,         /**< __mali_uku_profiling_add_event() */
	_MALI_UK_PROFILING_STOP,              /**< __mali_uku_profiling_stop() */
	_MALI_UK_PROFILING_GET_EVENT,         /**< __mali_uku_profiling_get_event() */
	_MALI_UK_PROFILING_CLEAR,             /**< __mali_uku_profiling_clear() */

#if USING_MALI_PMM
    /** Power Management Module Functions */
    _MALI_UK_PMM_EVENT_MESSAGE = 0,       /**< Raise an event message */
#endif
} _mali_uk_functions;

/**
 * ioctl commands
 */

#define MALI_IOC_BASE           0x82
#define MALI_IOC_CORE_BASE      (_MALI_UK_CORE_SUBSYSTEM      + MALI_IOC_BASE)
#define MALI_IOC_MEMORY_BASE    (_MALI_UK_MEMORY_SUBSYSTEM    + MALI_IOC_BASE)
#define MALI_IOC_PP_BASE        (_MALI_UK_PP_SUBSYSTEM        + MALI_IOC_BASE)
#define MALI_IOC_GP_BASE        (_MALI_UK_GP_SUBSYSTEM        + MALI_IOC_BASE)
#define MALI_IOC_PROFILING_BASE (_MALI_UK_PROFILING_SUBSYSTEM + MALI_IOC_BASE)

/**
 * ioctl commands
 */

#define MALI_IOC_GET_SYSTEM_INFO_SIZE       _IOR (MALI_IOC_CORE_BASE, _MALI_UK_GET_SYSTEM_INFO_SIZE, _mali_uk_get_system_info_s *)
#define MALI_IOC_GET_SYSTEM_INFO            _IOR (MALI_IOC_CORE_BASE, _MALI_UK_GET_SYSTEM_INFO, _mali_uk_get_system_info_s *)
#define MALI_IOC_WAIT_FOR_NOTIFICATION      _IOWR(MALI_IOC_CORE_BASE, _MALI_UK_WAIT_FOR_NOTIFICATION, _mali_uk_wait_for_notification_s *)
#define MALI_IOC_GET_API_VERSION            _IOWR(MALI_IOC_CORE_BASE, _MALI_UK_GET_API_VERSION, _mali_uk_get_api_version_s *)
#define MALI_IOC_MEM_GET_BIG_BLOCK          _IOWR(MALI_IOC_MEMORY_BASE, _MALI_UK_GET_BIG_BLOCK, _mali_uk_get_big_block_s *)
#define MALI_IOC_MEM_FREE_BIG_BLOCK         _IOW (MALI_IOC_MEMORY_BASE, _MALI_UK_FREE_BIG_BLOCK, _mali_uk_free_big_block_s *)
#define MALI_IOC_MEM_INIT                   _IOR (MALI_IOC_MEMORY_BASE, _MALI_UK_INIT_MEM, _mali_uk_init_mem_s *)
#define MALI_IOC_MEM_TERM                   _IOW (MALI_IOC_MEMORY_BASE, _MALI_UK_TERM_MEM, _mali_uk_term_mem_s *)
#define MALI_IOC_MEM_MAP_EXT                _IOWR(MALI_IOC_MEMORY_BASE, _MALI_UK_MAP_EXT_MEM, _mali_uk_map_external_mem_s *)
#define MALI_IOC_MEM_UNMAP_EXT              _IOW (MALI_IOC_MEMORY_BASE, _MALI_UK_UNMAP_EXT_MEM, _mali_uk_unmap_external_mem_s *)
#define MALI_IOC_MEM_QUERY_MMU_PAGE_TABLE_DUMP_SIZE _IOR (MALI_IOC_MEMORY_BASE, _MALI_UK_QUERY_MMU_PAGE_TABLE_DUMP_SIZE, _mali_uk_query_mmu_page_table_dump_size_s *)
#define MALI_IOC_MEM_DUMP_MMU_PAGE_TABLE    _IOWR(MALI_IOC_MEMORY_BASE, _MALI_UK_DUMP_MMU_PAGE_TABLE, _mali_uk_dump_mmu_page_table_s *)
#define MALI_IOC_MEM_ATTACH_UMP             _IOWR(MALI_IOC_MEMORY_BASE, _MALI_UK_ATTACH_UMP_MEM, _mali_uk_attach_ump_mem_s *)
#define MALI_IOC_MEM_RELEASE_UMP            _IOW(MALI_IOC_MEMORY_BASE, _MALI_UK_RELEASE_UMP_MEM, _mali_uk_release_ump_mem_s *)
#define MALI_IOC_PP_START_JOB               _IOWR(MALI_IOC_PP_BASE, _MALI_UK_PP_START_JOB, _mali_uk_pp_start_job_s *)
#define MALI_IOC_PP_NUMBER_OF_CORES_GET_R2P1	    _IOR (MALI_IOC_PP_BASE, _MALI_UK_GET_PP_NUMBER_OF_CORES_R2P1, _mali_uk_get_pp_number_of_cores_s *)
#define MALI_IOC_PP_CORE_VERSION_GET_R2P1	    _IOR (MALI_IOC_PP_BASE, _MALI_UK_GET_PP_CORE_VERSION_R2P1, _mali_uk_get_pp_core_version_s * )
#define MALI_IOC_PP_ABORT_JOB	            _IOW (MALI_IOC_PP_BASE, _MALI_UK_PP_ABORT_JOB, _mali_uk_pp_abort_job_s * )
#define MALI_IOC_PP_NUMBER_OF_CORES_GET_R3P0 _IOR (MALI_IOC_PP_BASE, _MALI_UK_GET_PP_NUMBER_OF_CORES_R3P0, _mali_uk_get_pp_number_of_cores_s *)
#define MALI_IOC_PP_CORE_VERSION_GET_R3P0    _IOR (MALI_IOC_PP_BASE, _MALI_UK_GET_PP_CORE_VERSION_R3P0, _mali_uk_get_pp_core_version_s * )
#define MALI_IOC_GP2_START_JOB              _IOWR(MALI_IOC_GP_BASE, _MALI_UK_GP_START_JOB, _mali_uk_gp_start_job_s *)
#define MALI_IOC_GP2_ABORT_JOB              _IOWR(MALI_IOC_GP_BASE, _MALI_UK_GP_ABORT_JOB, _mali_uk_gp_abort_job_s *)
#define MALI_IOC_GP2_NUMBER_OF_CORES_GET_R2P1    _IOR (MALI_IOC_GP_BASE, _MALI_UK_GET_GP_NUMBER_OF_CORES_R2P1, _mali_uk_get_gp_number_of_cores_s *)
#define MALI_IOC_GP2_CORE_VERSION_GET_R2P1	    _IOR (MALI_IOC_GP_BASE, _MALI_UK_GET_GP_CORE_VERSION_R2P1, _mali_uk_get_gp_core_version_s *)
#define MALI_IOC_GP2_NUMBER_OF_CORES_GET_R3P0 _IOR (MALI_IOC_GP_BASE, _MALI_UK_GET_GP_NUMBER_OF_CORES_R3P0, _mali_uk_get_gp_number_of_cores_s *)
#define MALI_IOC_GP2_CORE_VERSION_GET_R3P0    _IOR (MALI_IOC_GP_BASE, _MALI_UK_GET_GP_CORE_VERSION_R3P0, _mali_uk_get_gp_core_version_s *)

#define MALI_IOC_PROFILING_START            _IOWR(MALI_IOC_PROFILING_BASE, _MALI_UK_PROFILING_START, _mali_uk_profiling_start_s *)
#define MALI_IOC_PROFILING_ADD_EVENT        _IOWR(MALI_IOC_PROFILING_BASE, _MALI_UK_PROFILING_ADD_EVENT, _mali_uk_profiling_add_event_s*)
#define MALI_IOC_PROFILING_STOP             _IOWR(MALI_IOC_PROFILING_BASE, _MALI_UK_PROFILING_STOP, _mali_uk_profiling_stop_s *)
#define MALI_IOC_PROFILING_GET_EVENT        _IOWR(MALI_IOC_PROFILING_BASE, _MALI_UK_PROFILING_GET_EVENT, _mali_uk_profiling_get_event_s *)
#define MALI_IOC_PROFILING_CLEAR            _IOWR(MALI_IOC_PROFILING_BASE, _MALI_UK_PROFILING_CLEAR, _mali_uk_profiling_clear_s *)

/** @brief Get the size necessary for system info
 *
 * @see _mali_ukk_get_system_info_size()
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 size;                       /**< [out] size of buffer necessary to hold system information data, in bytes */
} _mali_uk_get_system_info_size_s;




/** @defgroup _mali_uk_getsysteminfo U/K Get System Info
 * @{ */

/**
 * Type definition for the core version number.
 * Used when returning the version number read from a core
 *
 * Its format is that of the 32-bit Version register for a particular core.
 * Refer to the "Mali200 and MaliGP2 3D Graphics Processor Technical Reference
 * Manual", ARM DDI 0415C, for more information.
 */
typedef u32 _mali_core_version;

/**
 * Enum values for the different modes the driver can be put in.
 * Normal is the default mode. The driver then uses a job queue and takes job objects from the clients.
 * Job completion is reported using the _mali_ukk_wait_for_notification call.
 * The driver blocks this io command until a job has completed or failed or a timeout occurs.
 *
 * The 'raw' mode is reserved for future expansion.
 */
typedef enum _mali_driver_mode
{
	_MALI_DRIVER_MODE_RAW = 1,    /**< Reserved for future expansion */
	_MALI_DRIVER_MODE_NORMAL = 2  /**< Normal mode of operation */
} _mali_driver_mode;

/** @brief List of possible cores
 *
 * add new entries to the end of this enum */
typedef enum _mali_core_type
{
	_MALI_GP2 = 2,                /**< MaliGP2 Programmable Vertex Processor */
	_MALI_200 = 5,                /**< Mali200 Programmable Fragment Processor */
	_MALI_400_GP = 6,             /**< Mali400 Programmable Vertex Processor */
	_MALI_400_PP = 7,             /**< Mali400 Programmable Fragment Processor */
	/* insert new core here, do NOT alter the existing values */
} _mali_core_type;

/** @brief Information about each Mali Core
 *
 * Information is stored in a linked list, which is stored entirely in the
 * buffer pointed to by the system_info member of the
 * _mali_uk_get_system_info_s arguments provided to _mali_ukk_get_system_info()
 *
 * Both Fragment Processor (PP) and Vertex Processor (GP) cores are represented
 * by this struct.
 *
 * The type is reported by the type field, _mali_core_info::_mali_core_type.
 *
 * Each core is given a unique Sequence number identifying it, the core_nr
 * member.
 *
 * Flags are taken directly from the resource's flags, and are currently unused.
 *
 * Multiple mali_core_info structs are linked in a single linked list using the next field
 */
typedef struct _mali_core_info
{
	_mali_core_type type;            /**< Type of core */
	_mali_core_version version;      /**< Core Version, as reported by the Core's Version Register */
	u32 reg_address;                 /**< Address of Registers */
	u32 core_nr;                     /**< Sequence number */
	u32 flags;                       /**< Flags. Currently Unused. */
	struct _mali_core_info * next;   /**< Next core in Linked List */
} _mali_core_info;

/** @brief Capabilities of Memory Banks
 *
 * These may be used to restrict memory banks for certain uses. They may be
 * used when access is not possible (e.g. Bus does not support access to it)
 * or when access is possible but not desired (e.g. Access is slow).
 *
 * In the case of 'possible but not desired', there is no way of specifying
 * the flags as an optimization hint, so that the memory could be used as a
 * last resort.
 *
 * @see _mali_mem_info
 */
typedef enum _mali_bus_usage
{

	_MALI_PP_READABLE   = (1<<0),  /** Readable by the Fragment Processor */
	_MALI_PP_WRITEABLE  = (1<<1),  /** Writeable by the Fragment Processor */
	_MALI_GP_READABLE   = (1<<2),  /** Readable by the Vertex Processor */
	_MALI_GP_WRITEABLE  = (1<<3),  /** Writeable by the Vertex Processor */
	_MALI_CPU_READABLE  = (1<<4),  /** Readable by the CPU */
	_MALI_CPU_WRITEABLE = (1<<5),  /** Writeable by the CPU */
	_MALI_MMU_READABLE  = _MALI_PP_READABLE | _MALI_GP_READABLE,   /** Readable by the MMU (including all cores behind it) */
	_MALI_MMU_WRITEABLE = _MALI_PP_WRITEABLE | _MALI_GP_WRITEABLE, /** Writeable by the MMU (including all cores behind it) */
} _mali_bus_usage;

/** @brief Information about the Mali Memory system
 *
 * Information is stored in a linked list, which is stored entirely in the
 * buffer pointed to by the system_info member of the
 * _mali_uk_get_system_info_s arguments provided to _mali_ukk_get_system_info()
 *
 * Each element of the linked list describes a single Mali Memory bank.
 * Each allocation can only come from one bank, and will not cross multiple
 * banks.
 *
 * Each bank is uniquely identified by its identifier member. On Mali-nonMMU
 * systems, to allocate from this bank, the value of identifier must be passed
 * as the type_id member of the  _mali_uk_get_big_block_s arguments to
 * _mali_ukk_get_big_block.
 *
 * On Mali-MMU systems, there is only one bank, which describes the maximum
 * possible address range that could be allocated (which may be much less than
 * the available physical memory)
 *
 * The flags member describes the capabilities of the memory. It is an error
 * to attempt to build a job for a particular core (PP or GP) when the memory
 * regions used do not have the capabilities for supporting that core. This
 * would result in a job abort from the Device Driver.
 *
 * For example, it is correct to build a PP job where read-only data structures
 * are taken from a memory with _MALI_PP_READABLE set and
 * _MALI_PP_WRITEABLE clear, and a framebuffer with  _MALI_PP_WRITEABLE set and
 * _MALI_PP_READABLE clear. However, it would be incorrect to use a framebuffer
 * where _MALI_PP_WRITEABLE is clear.
 */
typedef struct _mali_mem_info
{
	u32 size;                     /**< Size of the memory bank in bytes */
	_mali_bus_usage flags;        /**< Capabilitiy flags of the memory */
	u32 maximum_order_supported;  /**< log2 supported size */
	u32 identifier;               /**< Unique identifier, to be used in allocate calls */
	struct _mali_mem_info * next; /**< Next List Link */
} _mali_mem_info;

/** @brief Info about the whole Mali system.
 *
 * This Contains a linked list of the cores and memory banks available. Each
 * list pointer will remain inside the system_info buffer supplied in the
 * _mali_uk_get_system_info_s arguments to a _mali_ukk_get_system_info call.
 *
 * The has_mmu member must be inspected to ensure the correct group of
 * Memory function calls is obtained - that is, those for either Mali-MMU
 * or Mali-nonMMU. @see _mali_uk_memory
 */
typedef struct _mali_system_info
{
	_mali_core_info * core_info;  /**< List of _mali_core_info structures */
	_mali_mem_info * mem_info;    /**< List of _mali_mem_info structures */
	u32 has_mmu;                  /**< Non-zero if Mali-MMU present. Zero otherwise. */
	_mali_driver_mode drivermode; /**< Reserved. Must always be _MALI_DRIVER_MODE_NORMAL */
} _mali_system_info;

/** @brief Arguments to _mali_ukk_get_system_info()
 *
 * A buffer of the size returned by _mali_ukk_get_system_info_size() must be
 * allocated, and the pointer to this buffer must be written into the
 * system_info member. The buffer must be suitably aligned for storage of
 * the _mali_system_info structure - for example, one returned by
 * _mali_osk_malloc(), which will be suitably aligned for any structure.
 *
 * The ukk_private member must be set to zero by the user-side. Under an OS
 * implementation, the U/K interface must write in the user-side base address
 * into the ukk_private member, so that the common code in
 * _mali_ukk_get_system_info() can determine how to adjust the pointers such
 * that they are sensible from user space. Leaving ukk_private as NULL implies
 * that no pointer adjustment is necessary - which will be the case on a
 * bare-metal/RTOS system.
 *
 * @see _mali_system_info
 */
typedef struct
{
    void *ctx;                              /**< [in,out] user-kernel context (trashed on output) */
	u32 size;                               /**< [in] size of buffer provided to store system information data */
	_mali_system_info * system_info;        /**< [in,out] pointer to buffer to store system information data. No initialisation of buffer required on input. */
	u32 ukk_private;                        /**< [in] Kernel-side private word inserted by certain U/K interface implementations. Caller must set to Zero. */
} _mali_uk_get_system_info_s;
/** @} */ /* end group _mali_uk_getsysteminfo */

/** @} */ /* end group _mali_uk_core */





/** @defgroup _mali_uk_getapiversion_s Get API Version
 * @{ */

/** helpers for Device Driver API version handling */

/** @brief Encode a version ID from a 16-bit input
 *
 * @note the input is assumed to be 16 bits. It must not exceed 16 bits. */
#define _MAKE_VERSION_ID(x) (((x) << 16UL) | (x))

/** @brief Check whether a 32-bit value is likely to be Device Driver API
 * version ID. */
#define _IS_VERSION_ID(x) (((x) & 0xFFFF) == (((x) >> 16UL) & 0xFFFF))

/** @brief Decode a 16-bit version number from a 32-bit Device Driver API version
 * ID */
#define _GET_VERSION(x) (((x) >> 16UL) & 0xFFFF)

/** @brief Determine whether two 32-bit encoded version IDs match */
#define _IS_API_MATCH(x, y) (IS_VERSION_ID((x)) && IS_VERSION_ID((y)) && (GET_VERSION((x)) == GET_VERSION((y))))

/**
 * API version define.
 * Indicates the version of the kernel API
 * The version is a 16bit integer incremented on each API change.
 * The 16bit integer is stored twice in a 32bit integer
 * For example, for version 1 the value would be 0x00010001
 */
#define _MALI_API_VERSION 6
#define _MALI_UK_API_VERSION _MAKE_VERSION_ID(_MALI_API_VERSION)

/**
 * The API version is a 16-bit integer stored in both the lower and upper 16-bits
 * of a 32-bit value. The 16-bit API version value is incremented on each API
 * change. Version 1 would be 0x00010001. Used in _mali_uk_get_api_version_s.
 */
typedef uint32_t _mali_uk_api_version;

/** @brief Arguments for _mali_uk_get_api_version()
 *
 * The user-side interface version must be written into the version member,
 * encoded using _MAKE_VERSION_ID(). It will be compared to the API version of
 * the kernel-side interface.
 *
 * On successful return, the version member will be the API version of the
 * kernel-side interface. _MALI_UK_API_VERSION macro defines the current version
 * of the API.
 *
 * The compatible member must be checked to see if the version of the user-side
 * interface is compatible with the kernel-side interface, since future versions
 * of the interface may be backwards compatible.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	_mali_uk_api_version version;   /**< [in,out] API version of user-side interface. */
	int compatible;                 /**< [out] @c 1 when @version is compatible, @c 0 otherwise */
} _mali_uk_get_api_version_s;
/** @} */ /* end group _mali_uk_getapiversion_s */


/** @defgroup _mali_uk_memory U/K Memory
 * @{ */

/** @brief Arguments for _mali_ukk_init_mem(). */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 mali_address_base;          /**< [out] start of MALI address space */
	u32 memory_size;                /**< [out] total MALI address space available */
} _mali_uk_init_mem_s;

/** @defgroup _mali_uk_gpstartjob_s Vertex Processor Start Job
 * @{ */

/** @brief Status indicating the result of starting a Vertex or Fragment processor job */
typedef enum _mali_uk_start_job_status
{
    _MALI_UK_START_JOB_STARTED,                         /**< Job started */
    _MALI_UK_START_JOB_STARTED_LOW_PRI_JOB_RETURNED,    /**< Job started and bumped a lower priority job that was pending execution */
    _MALI_UK_START_JOB_NOT_STARTED_DO_REQUEUE           /**< Job could not be started at this time. Try starting the job again */
} _mali_uk_start_job_status;

/** @brief Status indicating the result of the execution of a Vertex or Fragment processor job  */

typedef enum
{
	_MALI_UK_JOB_STATUS_END_SUCCESS         = 1<<(16+0),
	_MALI_UK_JOB_STATUS_END_OOM             = 1<<(16+1),
	_MALI_UK_JOB_STATUS_END_ABORT           = 1<<(16+2),
	_MALI_UK_JOB_STATUS_END_TIMEOUT_SW      = 1<<(16+3),
	_MALI_UK_JOB_STATUS_END_HANG            = 1<<(16+4),
	_MALI_UK_JOB_STATUS_END_SEG_FAULT       = 1<<(16+5),
	_MALI_UK_JOB_STATUS_END_ILLEGAL_JOB     = 1<<(16+6),
	_MALI_UK_JOB_STATUS_END_UNKNOWN_ERR     = 1<<(16+7),
	_MALI_UK_JOB_STATUS_END_SHUTDOWN        = 1<<(16+8),
	_MALI_UK_JOB_STATUS_END_SYSTEM_UNUSABLE = 1<<(16+9)
} _mali_uk_job_status;

#define MALIGP2_NUM_REGS_FRAME (6)

/** @brief Arguments for _mali_ukk_gp_start_job()
 *
 * To start a Vertex Processor job
 * - associate the request with a reference to a @c mali_gp_job_info by setting
 * user_job_ptr to the address of the @c mali_gp_job_info of the job.
 * - set @c priority to the priority of the @c mali_gp_job_info
 * - specify a timeout for the job by setting @c watchdog_msecs to the number of
 * milliseconds the job is allowed to run. Specifying a value of 0 selects the
 * default timeout in use by the device driver.
 * - copy the frame registers from the @c mali_gp_job_info into @c frame_registers.
 * - set the @c perf_counter_flag, @c perf_counter_src0 and @c perf_counter_src1 to zero
 * for a non-instrumented build. For an instrumented build you can use up
 * to two performance counters. Set the corresponding bit in @c perf_counter_flag
 * to enable them. @c perf_counter_src0 and @c perf_counter_src1 specify
 * the source of what needs to get counted (e.g. number of vertex loader
 * cache hits). For source id values, see ARM DDI0415A, Table 3-60.
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 *
 * When @c _mali_ukk_gp_start_job() returns @c _MALI_OSK_ERR_OK, status contains the
 * result of the request (see \ref _mali_uk_start_job_status). If the job could
 * not get started (@c _MALI_UK_START_JOB_NOT_STARTED_DO_REQUEUE) it should be
 * tried again. If the job had a higher priority than the one currently pending
 * execution (@c _MALI_UK_START_JOB_STARTED_LOW_PRI_JOB_RETURNED), it will bump
 * the lower priority job and returns the address of the @c mali_gp_job_info
 * for that job in @c returned_user_job_ptr. That job should get requeued.
 *
 * After the job has started, @c _mali_wait_for_notification() will be notified
 * that the job finished or got suspended. It may get suspended due to
 * resource shortage. If it finished (see _mali_ukk_wait_for_notification())
 * the notification will contain a @c _mali_uk_gp_job_finished_s result. If
 * it got suspended the notification will contain a @c _mali_uk_gp_job_suspended_s
 * result.
 *
 * The @c _mali_uk_gp_job_finished_s contains the job status (see \ref _mali_uk_job_status),
 * the number of milliseconds the job took to render, and values of core registers
 * when the job finished (irq status, performance counters, renderer list
 * address). A job has finished succesfully when its status is
 * @c _MALI_UK_JOB_STATUS_FINISHED. If the hardware detected a timeout while rendering
 * the job, or software detected the job is taking more than watchdog_msecs to
 * complete, the status will indicate @c _MALI_UK_JOB_STATUS_HANG.
 * If the hardware detected a bus error while accessing memory associated with the
 * job, status will indicate @c _MALI_UK_JOB_STATUS_SEG_FAULT.
 * status will indicate @c _MALI_UK_JOB_STATUS_NOT_STARTED if the driver had to
 * stop the job but the job didn't start on the hardware yet, e.g. when the
 * driver shutdown.
 *
 * In case the job got suspended, @c _mali_uk_gp_job_suspended_s contains
 * the @c user_job_ptr identifier used to start the job with, the @c reason
 * why the job stalled (see \ref _maligp_job_suspended_reason) and a @c cookie
 * to identify the core on which the job stalled.  This @c cookie will be needed
 * when responding to this nofication by means of _mali_ukk_gp_suspend_response().
 * (see _mali_ukk_gp_suspend_response()). The response is either to abort or
 * resume the job. If the job got suspended due to an out of memory condition
 * you may be able to resolve this by providing more memory and resuming the job.
 *
 */
#if 0
typedef struct
{
    void *ctx;                          /**< [in,out] user-kernel context (trashed on output) */
    u32 user_job_ptr;                   /**< [in] identifier for the job in user space, a @c mali_gp_job_info* */
    u32 priority;                       /**< [in] job priority. A lower number means higher priority */
    u32 watchdog_msecs;                 /**< [in] maximum allowed runtime in milliseconds. The job gets killed if it runs longer than this. A value of 0 selects the default used by the device driver. */
    u32 frame_registers[MALIGP2_NUM_REGS_FRAME]; /**< [in] core specific registers associated with this job */
    u32 perf_counter_flag;              /**< [in] bitmask indicating which performance counters to enable, see \ref _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE and related macro definitions */
    u32 perf_counter_src0;              /**< [in] source id for performance counter 0 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter_src1;              /**< [in] source id for performance counter 1 (see ARM DDI0415A, Table 3-60) */
    u32 returned_user_job_ptr;          /**< [out] identifier for the returned job in user space, a @c mali_gp_job_info* */
    _mali_uk_start_job_status status;   /**< [out] indicates job start status (success, previous job returned, requeue) */
	u32 abort_id;                       /**< [in] abort id of this job, used to identify this job for later abort requests */
	u32 perf_counter_l2_src0;           /**< [in] soruce id for Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_src1;           /**< [in] source id for Mali-400 MP L2 cache performance counter 1 */
} _mali_uk_gp_start_job_s;
#endif

#define _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE (1<<0) /**< Enable performance counter SRC0 for a job */
#define _MALI_PERFORMANCE_COUNTER_FLAG_SRC1_ENABLE (1<<1) /**< Enable performance counter SRC1 for a job */
#define _MALI_PERFORMANCE_COUNTER_FLAG_L2_SRC0_ENABLE (1<<2) /**< Enable performance counter L2_SRC0 for a job */
#define _MALI_PERFORMANCE_COUNTER_FLAG_L2_SRC1_ENABLE (1<<3) /**< Enable performance counter L2_SRC1 for a job */
#define _MALI_PERFORMANCE_COUNTER_FLAG_L2_RESET       (1<<4) /**< Enable performance counter L2_RESET for a job */

/** @} */ /* end group _mali_uk_gpstartjob_s */

typedef struct
{
    u32 user_job_ptr;               /**< [out] identifier for the job in user space */
    _mali_uk_job_status status;     /**< [out] status of finished job */
    u32 irq_status;                 /**< [out] value of the GP interrupt rawstat register (see ARM DDI0415A) */
    u32 status_reg_on_stop;         /**< [out] value of the GP control register */
    u32 vscl_stop_addr;             /**< [out] value of the GP VLSCL start register */
    u32 plbcl_stop_addr;            /**< [out] value of the GP PLBCL start register */
    u32 heap_current_addr;          /**< [out] value of the GP PLB PL heap start address register */
    u32 perf_counter_src0;          /**< [out] source id for performance counter 0 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter_src1;          /**< [out] source id for performance counter 1 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter0;              /**< [out] value of perfomance counter 0 (see ARM DDI0415A) */
    u32 perf_counter1;              /**< [out] value of perfomance counter 1 (see ARM DDI0415A) */
    u32 render_time;                /**< [out] number of milliseconds it took for the job to render */
	u32 perf_counter_l2_src0;       /**< [out] soruce id for Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_src1;       /**< [out] soruce id for Mali-400 MP L2 cache performance counter 1 */
	u32 perf_counter_l2_val0;       /**< [out] Value of the Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_val1;       /**< [out] Value of the Mali-400 MP L2 cache performance counter 1 */
} _mali_uk_gp_job_finished_s;

typedef enum _maligp_job_suspended_reason
{
	_MALIGP_JOB_SUSPENDED_OUT_OF_MEMORY  /**< Polygon list builder unit (PLBU) has run out of memory */
} _maligp_job_suspended_reason;

typedef struct
{
	u32 user_job_ptr;                    /**< [out] identifier for the job in user space */
	_maligp_job_suspended_reason reason; /**< [out] reason why the job stalled */
	u32 cookie;                          /**< [out] identifier for the core in kernel space on which the job stalled */
} _mali_uk_gp_job_suspended_s;

/** @} */ /* end group _mali_uk_gp */

/** @defgroup _mali_uk_pp U/K Fragment Processor
 * @{ */

/** @defgroup _mali_uk_ppstartjob_s Fragment Processor Start Job
 * @{ */

/** @brief Arguments for _mali_ukk_pp_start_job()
 *
 * To start a Fragment Processor job
 * - associate the request with a reference to a mali_pp_job by setting
 * @c user_job_ptr to the address of the @c mali_pp_job of the job.
 * - set @c priority to the priority of the mali_pp_job
 * - specify a timeout for the job by setting @c watchdog_msecs to the number of
 * milliseconds the job is allowed to run. Specifying a value of 0 selects the
 * default timeout in use by the device driver.
 * - copy the frame registers from the @c mali_pp_job into @c frame_registers.
 * For MALI200 you also need to copy the write back 0,1 and 2 registers.
 * - set the @c perf_counter_flag, @c perf_counter_src0 and @c perf_counter_src1 to zero
 * for a non-instrumented build. For an instrumented build you can use up
 * to two performance counters. Set the corresponding bit in @c perf_counter_flag
 * to enable them. @c perf_counter_src0 and @c perf_counter_src1 specify
 * the source of what needs to get counted (e.g. number of vertex loader
 * cache hits). For source id values, see ARM DDI0415A, Table 3-60.
 * - pass in the user-kernel context in @c ctx that was returned from _mali_ukk_open()
 *
 * When _mali_ukk_pp_start_job() returns @c _MALI_OSK_ERR_OK, @c status contains the
 * result of the request (see \ref _mali_uk_start_job_status). If the job could
 * not get started (@c _MALI_UK_START_JOB_NOT_STARTED_DO_REQUEUE) it should be
 * tried again. If the job had a higher priority than the one currently pending
 * execution (@c _MALI_UK_START_JOB_STARTED_LOW_PRI_JOB_RETURNED), it will bump
 * the lower priority job and returns the address of the @c mali_pp_job
 * for that job in @c returned_user_job_ptr. That job should get requeued.
 *
 * After the job has started, _mali_wait_for_notification() will be notified
 * when the job finished. The notification will contain a
 * @c _mali_uk_pp_job_finished_s result. It contains the @c user_job_ptr
 * identifier used to start the job with, the job @c status (see \ref _mali_uk_job_status),
 * the number of milliseconds the job took to render, and values of core registers
 * when the job finished (irq status, performance counters, renderer list
 * address). A job has finished succesfully when its status is
 * @c _MALI_UK_JOB_STATUS_FINISHED. If the hardware detected a timeout while rendering
 * the job, or software detected the job is taking more than @c watchdog_msecs to
 * complete, the status will indicate @c _MALI_UK_JOB_STATUS_HANG.
 * If the hardware detected a bus error while accessing memory associated with the
 * job, status will indicate @c _MALI_UK_JOB_STATUS_SEG_FAULT.
 * status will indicate @c _MALI_UK_JOB_STATUS_NOT_STARTED if the driver had to
 * stop the job but the job didn't start on the hardware yet, e.g. when the
 * driver shutdown.
 *
 */
#if 0
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    u32 user_job_ptr;               /**< [in] identifier for the job in user space */
    u32 priority;                   /**< [in] job priority. A lower number means higher priority */
    u32 watchdog_msecs;             /**< [in] maximum allowed runtime in milliseconds. The job gets killed if it runs longer than this. A value of 0 selects the default used by the device driver. */
    u32 frame_registers[MALI200_NUM_REGS_FRAME]; /**< [in] core specific registers associated with this job, see ARM DDI0415A */
    u32 wb0_registers[MALI200_NUM_REGS_WBx];
    u32 wb1_registers[MALI200_NUM_REGS_WBx];
    u32 wb2_registers[MALI200_NUM_REGS_WBx];
    u32 perf_counter_flag;              /**< [in] bitmask indicating which performance counters to enable, see \ref _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE and related macro definitions */
    u32 perf_counter_src0;              /**< [in] source id for performance counter 0 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter_src1;              /**< [in] source id for performance counter 1 (see ARM DDI0415A, Table 3-60) */
    u32 returned_user_job_ptr;          /**< [out] identifier for the returned job in user space */
    _mali_uk_start_job_status status;   /**< [out] indicates job start status (success, previous job returned, requeue) */
	u32 abort_id;                       /**< [in] abort id of this job, used to identify this job for later abort requests */
	u32 perf_counter_l2_src0;           /**< [in] soruce id for Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_src1;           /**< [in] source id for Mali-400 MP L2 cache performance counter 1 */
} _mali_uk_pp_start_job_s;
#endif
/** @} */ /* end group _mali_uk_ppstartjob_s */

typedef struct
{
    u32 user_job_ptr;               /**< [out] identifier for the job in user space */
    _mali_uk_job_status status;     /**< [out] status of finished job */
    u32 irq_status;                 /**< [out] value of interrupt rawstat register (see ARM DDI0415A) */
    u32 last_tile_list_addr;        /**< [out] value of renderer list register (see ARM DDI0415A); necessary to restart a stopped job */
    u32 perf_counter_src0;          /**< [out] source id for performance counter 0 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter_src1;          /**< [out] source id for performance counter 1 (see ARM DDI0415A, Table 3-60) */
    u32 perf_counter0;              /**< [out] value of perfomance counter 0 (see ARM DDI0415A) */
    u32 perf_counter1;              /**< [out] value of perfomance counter 1 (see ARM DDI0415A) */
    u32 render_time;                /**< [out] number of milliseconds it took for the job to render */
	u32 perf_counter_l2_src0;       /**< [out] soruce id for Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_src1;       /**< [out] soruce id for Mali-400 MP L2 cache performance counter 1 */
	u32 perf_counter_l2_val0;       /**< [out] Value of the Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_val1;       /**< [out] Value of the Mali-400 MP L2 cache performance counter 1 */
	u32 perf_counter_l2_val0_raw;   /**< [out] Raw value of the Mali-400 MP L2 cache performance counter 0 */
	u32 perf_counter_l2_val1_raw;   /**< [out] Raw value of the Mali-400 MP L2 cache performance counter 1 */
} _mali_uk_pp_job_finished_s;
/** @} */ /* end group _mali_uk_pp */

/** @defgroup _mali_uk_waitfornotification_s Wait For Notification
 * @{ */

/** @brief Notification type encodings
 *
 * Each Notification type is an ordered pair of (subsystem,id), and is unique.
 *
 * The encoding of subsystem,id into a 32-bit word is:
 * encoding = (( subsystem << _MALI_NOTIFICATION_SUBSYSTEM_SHIFT ) & _MALI_NOTIFICATION_SUBSYSTEM_MASK)
 *            | (( id <<  _MALI_NOTIFICATION_ID_SHIFT ) & _MALI_NOTIFICATION_ID_MASK)
 *
 * @see _mali_uk_wait_for_notification_s
 */
typedef enum
{
	/** core notifications */

	_MALI_NOTIFICATION_CORE_TIMEOUT =               (_MALI_UK_CORE_SUBSYSTEM << 16) | 0x10,
	_MALI_NOTIFICATION_CORE_SHUTDOWN_IN_PROGRESS =  (_MALI_UK_CORE_SUBSYSTEM << 16) | 0x20,

	/** Fragment Processor notifications */

	_MALI_NOTIFICATION_PP_FINISHED =                (_MALI_UK_PP_SUBSYSTEM << 16) | 0x10,

	/** Vertex Processor notifications */

	_MALI_NOTIFICATION_GP_FINISHED =                (_MALI_UK_GP_SUBSYSTEM << 16) | 0x10,
	_MALI_NOTIFICATION_GP_STALLED =                 (_MALI_UK_GP_SUBSYSTEM << 16) | 0x20,
} _mali_uk_notification_type;

/** to assist in splitting up 32-bit notification value in subsystem and id value */
#define _MALI_NOTIFICATION_SUBSYSTEM_MASK 0xFFFF0000
#define _MALI_NOTIFICATION_SUBSYSTEM_SHIFT 16
#define _MALI_NOTIFICATION_ID_MASK 0x0000FFFF
#define _MALI_NOTIFICATION_ID_SHIFT 0


/** @brief Arguments for _mali_ukk_wait_for_notification()
 *
 * On successful return from _mali_ukk_wait_for_notification(), the members of
 * this structure will indicate the reason for notification.
 *
 * Specifically, the source of the notification can be identified by the
 * subsystem and id fields of the mali_uk_notification_type in the code.type
 * member. The type member is encoded in a way to divide up the types into a
 * subsystem field, and a per-subsystem ID field. See
 * _mali_uk_notification_type for more information.
 *
 * Interpreting the data union member depends on the notification type:
 *
 * - type == _MALI_NOTIFICATION_CORE_TIMEOUT
 *     - A notification timeout has occurred, since the code.timeout member was
 * exceeded.
 *     - In this case, the value of the data union member is undefined.
 *     - This is used so that the client can check other user-space state.
 * The client may repeat the call to _mali_ukk_wait_for_notification() to
 * continue reception of notifications.
 * - type == _MALI_NOTIFICATION_CORE_SHUTDOWN_IN_PROGRESS
 *     - The kernel side is shutting down. No further
 * _mali_uk_wait_for_notification() calls should be made.
 *     - In this case, the value of the data union member is undefined.
 *     - This is used to indicate to the user space client that it should close
 * the connection to the Mali Device Driver.
 * - type == _MALI_NOTIFICATION_PP_FINISHED
 *    - The notification data is of type _mali_uk_pp_job_finished_s. It contains the user_job_ptr
 * identifier used to start the job with, the job status, the number of milliseconds the job took to render,
 * and values of core registers when the job finished (irq status, performance counters, renderer list
 * address).
 *    - A job has finished succesfully when its status member is _MALI_UK_JOB_STATUS_FINISHED.
 *    - If the hardware detected a timeout while rendering the job, or software detected the job is
 * taking more than watchdog_msecs (see _mali_ukk_pp_start_job()) to complete, the status member will
 * indicate _MALI_UK_JOB_STATUS_HANG.
 *    - If the hardware detected a bus error while accessing memory associated with the job, status will
 * indicate _MALI_UK_JOB_STATUS_SEG_FAULT.
 *    - Status will indicate MALI_UK_JOB_STATUS_NOT_STARTED if the driver had to stop the job but the job
 * didn't start the hardware yet, e.g. when the driver closes.
 * - type == _MALI_NOTIFICATION_GP_FINISHED
 *     - The notification data is of type _mali_uk_gp_job_finished_s. The notification is similar to that of
 * type == _MALI_NOTIFICATION_PP_FINISHED, except that several other GP core register values are returned.
 * The status values have the same meaning for type == _MALI_NOTIFICATION_PP_FINISHED.
 * - type == _MALI_NOTIFICATION_GP_STALLED
 *     - The nofication data is of type _mali_uk_gp_job_suspended_s. It contains the user_job_ptr
 * identifier used to start the job with, the reason why the job stalled and a cookie to identify the core on
 * which the job stalled.
 *     - The reason member of gp_job_suspended is set to _MALIGP_JOB_SUSPENDED_OUT_OF_MEMORY
 * when the polygon list builder unit has run out of memory.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	union
	{
        /** [in] Number of milliseconds we should wait for a notification */
		u32 timeout;
        /** [out] Type of notification available */
		_mali_uk_notification_type type;
	} code;
    union
    {
        _mali_uk_gp_job_suspended_s gp_job_suspended;/**< [out] Notification data for _MALI_NOTIFICATION_GP_STALLED notification type */
        _mali_uk_gp_job_finished_s  gp_job_finished; /**< [out] Notification data for _MALI_NOTIFICATION_GP_FINISHED notification type */
        _mali_uk_pp_job_finished_s  pp_job_finished; /**< [out] Notification data for _MALI_NOTIFICATION_PP_FINISHED notification type */
    } data;
} _mali_uk_wait_for_notification_s;
/** @} */ /* end group _mali_uk_waitfornotification_s */


/** @brief Arguments for _mali_ukk_get_pp_number_of_cores()
 *
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 * - Upon successful return from _mali_ukk_get_pp_number_of_cores(), @c number_of_cores
 * will contain the number of Fragment Processor cores in the system.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    u32 number_of_cores;            /**< [out] number of Fragment Processor cores in the system */
} _mali_uk_get_pp_number_of_cores_s;

/** @brief Arguments for _mali_ukk_get_pp_core_version()
 *
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 * - Upon successful return from _mali_ukk_get_pp_core_version(), @c version contains
 * the version that all Fragment Processor cores are compatible with.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    _mali_core_version version;     /**< [out] version returned from core, see \ref _mali_core_version  */
} _mali_uk_get_pp_core_version_s;

/** @brief Arguments for _mali_ukk_get_gp_number_of_cores()
 *
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 * - Upon successful return from _mali_ukk_get_gp_number_of_cores(), @c number_of_cores
 * will contain the number of Vertex Processor cores in the system.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    u32 number_of_cores;            /**< [out] number of Vertex Processor cores in the system */
} _mali_uk_get_gp_number_of_cores_s;

/** @brief Arguments for _mali_ukk_get_gp_core_version()
 *
 * - pass in the user-kernel context @c ctx that was returned from _mali_ukk_open()
 * - Upon successful return from _mali_ukk_get_gp_core_version(), @c version contains
 * the version that all Vertex Processor cores are compatible with.
 */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
    _mali_core_version version;     /**< [out] version returned from core, see \ref _mali_core_version */
} _mali_uk_get_gp_core_version_s;

typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 phys_addr;                  /**< [in] physical address */
	u32 size;                       /**< [in] size */
	u32 mali_address;               /**< [in] mali address to map the physical memory to */
	u32 rights;                     /**< [in] rights necessary for accessing memory */
	u32 flags;                      /**< [in] flags, see \ref _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE */
	u32 cookie;                     /**< [out] identifier for mapped memory object in kernel space  */
} _mali_uk_map_external_mem_s;

/** Flag for _mali_uk_map_external_mem_s and _mali_uk_attach_ump_mem_s */
#define _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE (1<<0)

/** @note Mali-MMU only */
typedef struct
{
    void *ctx;                      /**< [in,out] user-kernel context (trashed on output) */
	u32 cookie;                     /**< [out] identifier for mapped memory object in kernel space  */
} _mali_uk_unmap_external_mem_s;
