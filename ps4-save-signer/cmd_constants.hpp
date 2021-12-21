#define PACKET_MAGIC 0x98696b77

#define UNEXPECTED_ERROR -1

#define CMD_STATUS_READY 0x70000001

#define CMD_MAGIC_MISMATCH 0x70100000
#define CMD_IS_INVALID 0x70100001
#define CMD_PARAMS_INVALID 0x70100002
#define CMD_UPLOAD_FILE_OPEN_FAILED 0x70110000


#define CMD_SAVE_GEN_MOUNT_ERROR 0x70120000
#define CMD_SAVE_GEN_UMOUNT_ERROR 0x70120001

#define CMD_SAVE_GEN_DELETE_MOUNT_ERROR 0x70120002
#define CMD_SAVE_GEN_TITLE_ID_UNSUPPORTED 0x70120003
#define CMD_SAVE_GEN_COPY_FOLDER_NOT_FOUND 0x70120004
#define CMD_SAVE_GEN_COPY_FOLDER_FAILED 0x70120005
#define CMD_SAVE_GEN_COPY_SKIP_FILE 0x70120006
#define CMD_SAVE_GEN_PARMSFO_MOD_FAILED 0x70120007

#define CMD_UPLOAD_FILE 0x70200000
#define CMD_SAVE_GEN 0x70200001
#define CMD_DELETE_UPLOAD_DIRECTORY 0x70200003