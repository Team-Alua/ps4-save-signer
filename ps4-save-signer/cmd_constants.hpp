#define PACKET_MAGIC 0x98696b77

#define UNEXPECTED_ERROR -1

#define CMD_STATUS_READY 0x80000001

#define CMD_MAGIC_MISMATCH 0x80100000
#define CMD_IS_INVALID 0x80100001
#define CMD_PARAMS_INVALID 0x80100002
#define CMD_UPLOAD_FILE_OPEN_FAILED 0x80110000


#define CMD_SAVE_GEN_MOUNT_ERROR 0x80120000
#define CMD_SAVE_GEN_UMOUNT_ERROR 0x80120001

#define CMD_SAVE_GEN_DELETE_MOUNT_ERROR 0x80120002
#define CMD_SAVE_GEN_TITLE_ID_UNSUPPORTED 0x80120003
#define CMD_SAVE_GEN_COPY_FOLDER_NOT_FOUND 0x80120004
#define CMD_SAVE_GEN_COPY_FOLDER_FAILED 0x80120005
#define CMD_SAVE_GEN_COPY_SKIP_FILE 0x80120006
#define CMD_SAVE_GEN_PARMSFO_MOD_FAILED 0x80120007

#define CMD_UPLOAD_FILE 0x80200000
#define CMD_SAVE_GEN 0x80200001