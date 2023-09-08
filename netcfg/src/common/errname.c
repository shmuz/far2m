#include <errno.h>

// for duplicates
#if !defined(__APPLE__) && !defined(__FreeBSD__)
#define	_EWOULDBLOCK	41	/* Operation would block */
#define	_EDEADLOCK	58

#ifndef ENOKEY
#define	ENOKEY		126	/* Required key not available */
#define	EKEYEXPIRED	127	/* Key has expired */
#define	EKEYREVOKED	128	/* Key has been revoked */
#define	EKEYREJECTED	129	/* Key was rejected by service */
#define	EOWNERDEAD	130	/* Owner died */
#define	ENOTRECOVERABLE	131	/* State not recoverable */
#define ERFKILL		132	/* Operation not possible due to RF-kill */
#define EHWPOISON	133	/* Memory page has hardware error */
#endif

#endif

#define CASE_ERR(err) \
	case err: return #err

static const char *itoa(int i) {
	static char buf[12];
	char *ptr = buf + sizeof(buf) - 1;
	unsigned int u;
	int minus = 0;
	if (i < 0) {
		minus = 1;
		u = ((unsigned int)(-(1+i))) + 1;
	} else
		u = i;
	*ptr = 0;
	do {
	    *--ptr = '0' + (u % 10);
	} while(u/=10);

	if (minus)
		*--ptr = '-';
	return ptr;
}

#ifdef __cplusplus
extern "C" {
#endif

const char * errorname(int err)
{
	switch (err) {
	case 0: return "SUCCESS";
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	CASE_ERR(EPERM		);/* Operation not permitted */
	CASE_ERR(ENOENT		);/* No such file or directory */
	CASE_ERR(ESRCH		);/* No such process */
	CASE_ERR(EINTR		);/* Interrupted system call */
	CASE_ERR(EIO		);/* I/O error */
	CASE_ERR(ENXIO		);/* No such device or address */
	CASE_ERR(E2BIG		);/* Argument list too long */
	CASE_ERR(ENOEXEC	);/* Exec format error */
	CASE_ERR(EBADF		);/* Bad file number */
	CASE_ERR(ECHILD		);/* No child processes */
	CASE_ERR(EAGAIN		);/* Try again */
	CASE_ERR(ENOMEM		);/* Out of memory */
	CASE_ERR(EACCES		);/* Permission denied */
	CASE_ERR(EFAULT		);/* Bad address */
	CASE_ERR(ENOTBLK	);/* Block device required */
	CASE_ERR(EBUSY		);/* Device or resource busy */
	CASE_ERR(EEXIST		);/* File exists */
	CASE_ERR(EXDEV		);/* Cross-device link */
	CASE_ERR(ENODEV		);/* No such device */
	CASE_ERR(ENOTDIR	);/* Not a directory */
	CASE_ERR(EISDIR		);/* Is a directory */
	CASE_ERR(EINVAL		);/* Invalid argument */
	CASE_ERR(ENFILE		);/* File table overflow */
	CASE_ERR(EMFILE		);/* Too many open files */
	CASE_ERR(ENOTTY		);/* Not a typewriter */
	CASE_ERR(ETXTBSY	);/* Text file busy */
	CASE_ERR(EFBIG		);/* File too large */
	CASE_ERR(ENOSPC		);/* No space left on device */
	CASE_ERR(ESPIPE		);/* Illegal seek */
	CASE_ERR(EROFS		);/* Read-only file system */
	CASE_ERR(EMLINK		);/* Too many links */
	CASE_ERR(EPIPE		);/* Broken pipe */
	CASE_ERR(EDOM		);/* Math argument out of domain of func */
	CASE_ERR(ERANGE		);/* Math result not representable */
	CASE_ERR(EDEADLK	);/* Resource deadlock would occur */
	CASE_ERR(ENAMETOOLONG	);/* File name too long */
	CASE_ERR(ENOLCK		);/* No record locks available */
	CASE_ERR(ENOSYS		);/* Invalid system call number */
	CASE_ERR(ENOTEMPTY	);/* Directory not empty */
	CASE_ERR(ELOOP		);/* Too many symbolic links encountered */
#if ELNRNG != 41
	CASE_ERR(_EWOULDBLOCK	);/* Operation would block */
#endif
	CASE_ERR(ENOMSG		);/* No message of desired type */
	CASE_ERR(EIDRM		);/* Identifier removed */
	CASE_ERR(ECHRNG		);/* Channel number out of range */
	CASE_ERR(EL2NSYNC	);/* Level 2 not synchronized */
	CASE_ERR(EL3HLT		);/* Level 3 halted */
	CASE_ERR(EL3RST		);/* Level 3 reset */
	CASE_ERR(ELNRNG		);/* Link number out of range */
	CASE_ERR(EUNATCH	);/* Protocol driver not attached */
	CASE_ERR(ENOCSI		);/* No CSI structure available */
	CASE_ERR(EL2HLT		);/* Level 2 halted */
	CASE_ERR(EBADE		);/* Invalid exchange */
	CASE_ERR(EBADR		);/* Invalid request descriptor */
	CASE_ERR(EXFULL		);/* Exchange full */
	CASE_ERR(ENOANO		);/* No anode */
	CASE_ERR(EBADRQC	);/* Invalid request code */
	CASE_ERR(EBADSLT	);/* Invalid slot */
	CASE_ERR(_EDEADLOCK	);/* Resource deadlock would occur */
	CASE_ERR(EBFONT		);/* Bad font file format */
	CASE_ERR(ENOSTR		);/* Device not a stream */
	CASE_ERR(ENODATA	);/* No data available */
	CASE_ERR(ETIME		);/* Timer expired */
	CASE_ERR(ENOSR		);/* Out of streams resources */
	CASE_ERR(ENONET		);/* Machine is not on the network */
	CASE_ERR(ENOPKG		);/* Package not installed */
	CASE_ERR(EREMOTE	);/* Object is remote */
	CASE_ERR(ENOLINK	);/* Link has been severed */
	CASE_ERR(EADV		);/* Advertise error */
	CASE_ERR(ESRMNT		);/* Srmount error */
	CASE_ERR(ECOMM		);/* Communication error on send */
	CASE_ERR(EPROTO		);/* Protocol error */
	CASE_ERR(EMULTIHOP	);/* Multihop attempted */
	CASE_ERR(EDOTDOT	);/* RFS specific error */
	CASE_ERR(EBADMSG	);/* Not a data message */
	CASE_ERR(EOVERFLOW	);/* Value too large for defined data type */
	CASE_ERR(ENOTUNIQ	);/* Name not unique on network */
	CASE_ERR(EBADFD		);/* File descriptor in bad state */
	CASE_ERR(EREMCHG	);/* Remote address changed */
	CASE_ERR(ELIBACC	);/* Can not access a needed shared library */
	CASE_ERR(ELIBBAD	);/* Accessing a corrupted shared library */
	CASE_ERR(ELIBSCN	);/* .lib section in a.out corrupted */
	CASE_ERR(ELIBMAX	);/* Attempting to link in too many shared libraries */
	CASE_ERR(ELIBEXEC	);/* Cannot exec a shared library directly */
	CASE_ERR(EILSEQ		);/* Illegal byte sequence */
	CASE_ERR(ERESTART	);/* Interrupted system call should be restarted */
	CASE_ERR(ESTRPIPE	);/* Streams pipe error */
	CASE_ERR(EUSERS		);/* Too many users */
	CASE_ERR(ENOTSOCK	);/* Socket operation on non-socket */
	CASE_ERR(EDESTADDRREQ	);/* Destination address required */
	CASE_ERR(EMSGSIZE	);/* Message too long */
	CASE_ERR(EPROTOTYPE	);/* Protocol wrong type for socket */
	CASE_ERR(ENOPROTOOPT	);/* Protocol not available */
	CASE_ERR(EPROTONOSUPPORT);/* Protocol not supported */
	CASE_ERR(ESOCKTNOSUPPORT);/* Socket type not supported */
	CASE_ERR(EOPNOTSUPP	);/* Operation not supported on transport endpoint */
	CASE_ERR(EPFNOSUPPORT	);/* Protocol family not supported */
	CASE_ERR(EAFNOSUPPORT	);/* Address family not supported by protocol */
	CASE_ERR(EADDRINUSE	);/* Address already in use */
	CASE_ERR(EADDRNOTAVAIL	);/* Cannot assign requested address */
	CASE_ERR(ENETDOWN	);/* Network is down */
	CASE_ERR(ENETUNREACH	);/* Network is unreachable */
	CASE_ERR(ENETRESET	);/* Network dropped connection because of reset */
	CASE_ERR(ECONNABORTED	);/* Software caused connection abort */
	CASE_ERR(ECONNRESET	);/* Connection reset by peer */
	CASE_ERR(ENOBUFS	);/* No buffer space available */
	CASE_ERR(EISCONN	);/* Transport endpoint is already connected */
	CASE_ERR(ENOTCONN	);/* Transport endpoint is not connected */
	CASE_ERR(ESHUTDOWN	);/* Cannot send after transport endpoint shutdown */
	CASE_ERR(ETOOMANYREFS	);/* Too many references: cannot splice */
	CASE_ERR(ETIMEDOUT	);/* Connection timed out */
	CASE_ERR(ECONNREFUSED	);/* Connection refused */
	CASE_ERR(EHOSTDOWN	);/* Host is down */
	CASE_ERR(EHOSTUNREACH	);/* No route to host */
	CASE_ERR(EALREADY	);/* Operation already in progress */
	CASE_ERR(EINPROGRESS	);/* Operation now in progress */
	CASE_ERR(ESTALE		);/* Stale file handle */
	CASE_ERR(EUCLEAN	);/* Structure needs cleaning */
	CASE_ERR(ENOTNAM	);/* Not a XENIX named type file */
	CASE_ERR(ENAVAIL	);/* No XENIX semaphores available */
	CASE_ERR(EISNAM		);/* Is a named type file */
	CASE_ERR(EREMOTEIO	);/* Remote I/O error */
	CASE_ERR(EDQUOT		);/* Quota exceeded */
	CASE_ERR(ENOMEDIUM	);/* No medium found */
	CASE_ERR(EMEDIUMTYPE	);/* Wrong medium type */
	CASE_ERR(ECANCELED	);/* Operation Canceled */
#if EADDRNOTAVAIL != ENOKEY
	CASE_ERR(ENOKEY		);/* Required key not available */
	CASE_ERR(EKEYEXPIRED	);/* Key has expired */
	CASE_ERR(EKEYREVOKED	);/* Key has been revoked */
	CASE_ERR(EKEYREJECTED	);/* Key was rejected by service */
	CASE_ERR(EOWNERDEAD	);/* Owner died */
	CASE_ERR(ENOTRECOVERABLE);/* State not recoverable */
	CASE_ERR(ERFKILL	);/* Operation not possible due to RF-kill */
	CASE_ERR(EHWPOISON	);/* Memory page has hardware error */
#endif

#else
	CASE_ERR(EPERM          );/* Operation not permitted */
	CASE_ERR(ENOENT         );/* No such file or directory */
	CASE_ERR(ESRCH          );/* No such process */
	CASE_ERR(EINTR          );/* Interrupted system call */
	CASE_ERR(EIO            );/* Input/output error */
	CASE_ERR(ENXIO          );/* Device not configured */
	CASE_ERR(E2BIG          );/* Argument list too long */
	CASE_ERR(ENOEXEC        );/* Exec format error */
	CASE_ERR(EBADF          );/* Bad file descriptor */
	CASE_ERR(ECHILD         );/* No child processes */
	CASE_ERR(EDEADLK        );/* Resource deadlock avoided */
	CASE_ERR(ENOMEM         );/* Cannot allocate memory */
	CASE_ERR(EACCES         );/* Permission denied */
	CASE_ERR(EFAULT         );/* Bad address */
	CASE_ERR(ENOTBLK        );/* Block device required */
	CASE_ERR(EBUSY          );/* Device / Resource busy */
	CASE_ERR(EEXIST         );/* File exists */
	CASE_ERR(EXDEV          );/* Cross-device link */
	CASE_ERR(ENODEV         );/* Operation not supported by device */
	CASE_ERR(ENOTDIR        );/* Not a directory */
	CASE_ERR(EISDIR         );/* Is a directory */
	CASE_ERR(EINVAL         );/* Invalid argument */
	CASE_ERR(ENFILE         );/* Too many open files in system */
	CASE_ERR(EMFILE         );/* Too many open files */
	CASE_ERR(ENOTTY         );/* Inappropriate ioctl for device */
	CASE_ERR(ETXTBSY        );/* Text file busy */
	CASE_ERR(EFBIG          );/* File too large */
	CASE_ERR(ENOSPC         );/* No space left on device */
	CASE_ERR(ESPIPE         );/* Illegal seek */
	CASE_ERR(EROFS          );/* Read-only file system */
	CASE_ERR(EMLINK         );/* Too many links */
	CASE_ERR(EPIPE          );/* Broken pipe */
	CASE_ERR(EDOM           );/* Numerical argument out of domain */
	CASE_ERR(ERANGE         );/* Result too large */
	CASE_ERR(EAGAIN         );/* Resource temporarily unavailable */
	CASE_ERR(EINPROGRESS    );/* Operation now in progress */
	CASE_ERR(EALREADY       );/* Operation already in progress */
	CASE_ERR(ENOTSOCK       );/* Socket operation on non-socket */
	CASE_ERR(EDESTADDRREQ   );/* Destination address required */
	CASE_ERR(EMSGSIZE       );/* Message too long */
	CASE_ERR(EPROTOTYPE     );/* Protocol wrong type for socket */
	CASE_ERR(ENOPROTOOPT    );/* Protocol not available */
	CASE_ERR(EPROTONOSUPPORT);/* Protocol not supported */
	CASE_ERR(ESOCKTNOSUPPORT);/* Socket type not supported */
	CASE_ERR(ENOTSUP        );/* Operation not supported */
	CASE_ERR(EPFNOSUPPORT   );/* Protocol family not supported */
	CASE_ERR(EAFNOSUPPORT   );/* Address family not supported by protocol family */
	CASE_ERR(EADDRINUSE     );/* Address already in use */
	CASE_ERR(EADDRNOTAVAIL  );/* Can't assign requested address */
	CASE_ERR(ENETDOWN       );/* Network is down */
	CASE_ERR(ENETUNREACH    );/* Network is unreachable */
	CASE_ERR(ENETRESET      );/* Network dropped connection on reset */
	CASE_ERR(ECONNABORTED   );/* Software caused connection abort */
	CASE_ERR(ECONNRESET     );/* Connection reset by peer */
	CASE_ERR(ENOBUFS        );/* No buffer space available */
	CASE_ERR(EISCONN        );/* Socket is already connected */
	CASE_ERR(ENOTCONN       );/* Socket is not connected */
	CASE_ERR(ESHUTDOWN      );/* Can't send after socket shutdown */
	CASE_ERR(ETOOMANYREFS   );/* Too many references: can't splice */
	CASE_ERR(ETIMEDOUT      );/* Operation timed out */
	CASE_ERR(ECONNREFUSED   );/* Connection refused */
	CASE_ERR(ELOOP          );/* Too many levels of symbolic links */
	CASE_ERR(ENAMETOOLONG   );/* File name too long */
	CASE_ERR(EHOSTDOWN      );/* Host is down */
	CASE_ERR(EHOSTUNREACH   );/* No route to host */
	CASE_ERR(ENOTEMPTY      );/* Directory not empty */
	CASE_ERR(EPROCLIM       );/* Too many processes */
	CASE_ERR(EUSERS         );/* Too many users */
	CASE_ERR(EDQUOT         );/* Disc quota exceeded */
	CASE_ERR(ESTALE         );/* Stale NFS file handle */
	CASE_ERR(EREMOTE        );/* Too many levels of remote in path */
	CASE_ERR(EBADRPC        );/* RPC struct is bad */
	CASE_ERR(ERPCMISMATCH   );/* RPC version wrong */
	CASE_ERR(EPROGUNAVAIL   );/* RPC prog. not avail */
	CASE_ERR(EPROGMISMATCH  );/* Program version wrong */
	CASE_ERR(EPROCUNAVAIL   );/* Bad procedure for program */
	CASE_ERR(ENOLCK         );/* No locks available */
	CASE_ERR(ENOSYS         );/* Function not implemented */
	CASE_ERR(EFTYPE         );/* Inappropriate file type or format */
	CASE_ERR(EAUTH          );/* Authentication error */
	CASE_ERR(ENEEDAUTH      );/* Need authenticator */
	CASE_ERR(EPWROFF        );/* Device power is off */
	CASE_ERR(EDEVERR        );/* Device error, e.g. paper out */
	CASE_ERR(EOVERFLOW      );/* Value too large to be stored in data type */
	CASE_ERR(EBADEXEC       );/* Bad executable */
	CASE_ERR(EBADARCH       );/* Bad CPU type in executable */
	CASE_ERR(ESHLIBVERS     );/* Shared library version mismatch */
	CASE_ERR(EBADMACHO      );/* Malformed Macho file */
	CASE_ERR(ECANCELED      );/* Operation canceled */
	CASE_ERR(EIDRM          );/* Identifier removed */
	CASE_ERR(ENOMSG         );/* No message of desired type */
	CASE_ERR(EILSEQ         );/* Illegal byte sequence */
	CASE_ERR(ENOATTR        );/* Attribute not found */
	CASE_ERR(EBADMSG        );/* Bad message */
	CASE_ERR(EMULTIHOP      );/* Reserved */
	CASE_ERR(ENODATA        );/* No message available on STREAM */
	CASE_ERR(ENOLINK        );/* Reserved */
	CASE_ERR(ENOSR          );/* No STREAM resources */
	CASE_ERR(ENOSTR         );/* Not a STREAM */
	CASE_ERR(EPROTO         );/* Protocol error */
	CASE_ERR(ETIME          );/* STREAM ioctl timeout */
	CASE_ERR(EOPNOTSUPP     );/* Operation not supported on socket */
	CASE_ERR(ENOPOLICY      );/* No such policy registered */
	CASE_ERR(ENOTRECOVERABLE);/* State not recoverable */
	CASE_ERR(EOWNERDEAD     );/* Previous owner died */
	CASE_ERR(EQFULL         );/* Interface output queue is full */
#endif
	default:
		return itoa(err);
	}
}

#ifdef __cplusplus
}
#endif


#ifdef MAIN_COMMON_ERRNAME

#include "log.h"

#define LOG_FILE ""
#define LOG_SOURCE_FILE "errname.c"

int main(int argc, char * argv[])
{
	LOG_ERROR_CONSOLE("%u (%s)\n", 0, errorname(0));
	LOG_ERROR_CONSOLE("%u (%s)\n", EPERM, errorname(EPERM));
	LOG_ERROR_CONSOLE("%u (%s)\n", ENOENT, errorname(ENOENT));
	LOG_ERROR_CONSOLE("%u (%s)\n", EOWNERDEAD, errorname(EOWNERDEAD));
	LOG_ERROR_CONSOLE("%u (%s)\n", 33333, errorname(33333));
	return 0;
}
#endif // MAIN_COMMON_ERRNAME
