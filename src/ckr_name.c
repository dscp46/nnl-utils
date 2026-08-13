
#include "ckr_name.h"
 
const char *ckr_name( CK_RV rv)
{
	switch (rv) {
	case CKR_OK:                           return "CKR_OK";
	case CKR_CANCEL:                       return "CKR_CANCEL";
	case CKR_HOST_MEMORY:                  return "CKR_HOST_MEMORY";
	case CKR_SLOT_ID_INVALID:              return "CKR_SLOT_ID_INVALID";
	case CKR_GENERAL_ERROR:                return "CKR_GENERAL_ERROR";
	case CKR_FUNCTION_FAILED:              return "CKR_FUNCTION_FAILED";
	case CKR_ARGUMENTS_BAD:                return "CKR_ARGUMENTS_BAD";
	case CKR_NO_EVENT:                     return "CKR_NO_EVENT";
	case CKR_ATTRIBUTE_READ_ONLY:          return "CKR_ATTRIBUTE_READ_ONLY";
	case CKR_ATTRIBUTE_TYPE_INVALID:       return "CKR_ATTRIBUTE_TYPE_INVALID";
	case CKR_ATTRIBUTE_VALUE_INVALID:      return "CKR_ATTRIBUTE_VALUE_INVALID";
	case CKR_DATA_INVALID:                 return "CKR_DATA_INVALID";
	case CKR_DATA_LEN_RANGE:               return "CKR_DATA_LEN_RANGE";
	case CKR_DEVICE_ERROR:                 return "CKR_DEVICE_ERROR";
	case CKR_DEVICE_MEMORY:                return "CKR_DEVICE_MEMORY";
	case CKR_DEVICE_REMOVED:               return "CKR_DEVICE_REMOVED";
	case CKR_ENCRYPTED_DATA_INVALID:       return "CKR_ENCRYPTED_DATA_INVALID";
	case CKR_ENCRYPTED_DATA_LEN_RANGE:     return "CKR_ENCRYPTED_DATA_LEN_RANGE";
	case CKR_KEY_HANDLE_INVALID:           return "CKR_KEY_HANDLE_INVALID";
	case CKR_KEY_SIZE_RANGE:               return "CKR_KEY_SIZE_RANGE";
	case CKR_KEY_TYPE_INCONSISTENT:        return "CKR_KEY_TYPE_INCONSISTENT";
	case CKR_KEY_FUNCTION_NOT_PERMITTED:   return "CKR_KEY_FUNCTION_NOT_PERMITTED";
	case CKR_MECHANISM_INVALID:            return "CKR_MECHANISM_INVALID";
	case CKR_MECHANISM_PARAM_INVALID:      return "CKR_MECHANISM_PARAM_INVALID";
	case CKR_OPERATION_ACTIVE:             return "CKR_OPERATION_ACTIVE";
	case CKR_OPERATION_NOT_INITIALIZED:    return "CKR_OPERATION_NOT_INITIALIZED";
	case CKR_PIN_INCORRECT:                return "CKR_PIN_INCORRECT";
	case CKR_PIN_INVALID:                  return "CKR_PIN_INVALID";
	case CKR_PIN_LEN_RANGE:                return "CKR_PIN_LEN_RANGE";
	case CKR_PIN_EXPIRED:                  return "CKR_PIN_EXPIRED";
	case CKR_PIN_LOCKED:                   return "CKR_PIN_LOCKED";
	case CKR_SESSION_CLOSED:               return "CKR_SESSION_CLOSED";
	case CKR_SESSION_COUNT:                return "CKR_SESSION_COUNT";
	case CKR_SESSION_HANDLE_INVALID:       return "CKR_SESSION_HANDLE_INVALID";
	case CKR_SESSION_READ_ONLY:            return "CKR_SESSION_READ_ONLY";
	case CKR_SESSION_EXISTS:               return "CKR_SESSION_EXISTS";
	case CKR_TOKEN_NOT_PRESENT:            return "CKR_TOKEN_NOT_PRESENT";
	case CKR_TOKEN_NOT_RECOGNIZED:         return "CKR_TOKEN_NOT_RECOGNIZED";
	case CKR_TOKEN_WRITE_PROTECTED:        return "CKR_TOKEN_WRITE_PROTECTED";
	case CKR_USER_ALREADY_LOGGED_IN:       return "CKR_USER_ALREADY_LOGGED_IN";
	case CKR_USER_NOT_LOGGED_IN:           return "CKR_USER_NOT_LOGGED_IN";
	case CKR_USER_PIN_NOT_INITIALIZED:     return "CKR_USER_PIN_NOT_INITIALIZED";
	case CKR_USER_TYPE_INVALID:            return "CKR_USER_TYPE_INVALID";
	case CKR_USER_TOO_MANY_TYPES:          return "CKR_USER_TOO_MANY_TYPES";
	case CKR_CRYPTOKI_NOT_INITIALIZED:     return "CKR_CRYPTOKI_NOT_INITIALIZED";
	case CKR_CRYPTOKI_ALREADY_INITIALIZED: return "CKR_CRYPTOKI_ALREADY_INITIALIZED";
	case CKR_BUFFER_TOO_SMALL:             return "CKR_BUFFER_TOO_SMALL";
	case CKR_FUNCTION_NOT_SUPPORTED:       return "CKR_FUNCTION_NOT_SUPPORTED";
	default:
		if(rv & CKR_VENDOR_DEFINED)
			return "CKR_VENDOR_DEFINED";
		return "CKR_(unknown)";
	}
}
