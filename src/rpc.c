#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpc.pb-c.h"

bool invert(bool v)
{
  return !v;
}

int invertWrapper(uint8_t **valueSerial, size_t *valueSerialLen, const uint8_t *argsSerial, size_t argsSerialLen)
{
  int error = 0;

  // Deserialize/Unpack the arguments
  InvertArguments *args = invert_arguments__unpack(NULL, argsSerialLen, argsSerial);
  if(args == NULL) {
    return 1;
  }

  // Call the underlying function
  bool notv = invert(args->v);

  // Serialize/Pack the return value
  InvertReturnValue value = INVERT_RETURN_VALUE__INIT;
  value.notv = notv;

  *valueSerialLen = invert_return_value__get_packed_size(&value);
  *valueSerial = (uint8_t *)malloc(*valueSerialLen);
  if (*valueSerial == NULL) {
    error = 1;
    goto errValueMalloc;
  }
  
  invert_return_value__pack(&value, *valueSerial);

  // Cleanup
errValueMalloc:
  invert_arguments__free_unpacked(args, NULL);

  return error;
}

int handleCall(uint8_t **retSerial, size_t *retSerialLen, const uint8_t *callSerial, size_t callSerialLen)
{
  int error = 0;

  // Deserializing/Unpacking the call
  Call *call = call__unpack(NULL, callSerialLen, callSerial);
  if (call == NULL) {
    return 1;
  }

  // Calling the appropriate wrapper function based on the `name' field
  uint8_t *valueSerial;
  size_t valueSerialLen;
  bool success;

  if (strcmp(call->name, "invert") == 0) {
    success = !invertWrapper(&valueSerial, &valueSerialLen, call->args.data, call->args.len);
  } else {
    error = 1;
    goto errInvalidName;
  }

  // Serializing/Packing the return, using the return value from the invoked function
  Return ret = RETURN__INIT;
  ret.success = success;
  if (success) {
    ret.value.data = valueSerial;
    ret.value.len = valueSerialLen;
  }

  *retSerialLen = return__get_packed_size(&ret);
  *retSerial = (uint8_t *)malloc(*retSerialLen);
  if (*retSerial == NULL) {
    error = 1;
    goto errRetMalloc;
  }
  
  return__pack(&ret, *retSerial);

  // Cleanup
errRetMalloc:
  if (success) {
    free(valueSerial);
  }
errInvalidName:
  call__free_unpacked(call, NULL);

  return error;
}

int callInvert(bool *notv, bool v)
{
  int error = 0;

  // Serializing/Packing the arguments
  InvertArguments args = INVERT_ARGUMENTS__INIT;
  args.v = v;

  size_t argsSerialLen = invert_arguments__get_packed_size(&args);
  uint8_t *argsSerial = (uint8_t *)malloc(argsSerialLen);
  if (argsSerial == NULL) {
    return 1;
  }

  invert_arguments__pack(&args, argsSerial);

  // Serializing/Packing the call, which also placing the serialized/packed
  // arguments inside
  Call call = CALL__INIT;
  call.name = "invert";
  call.args.len = argsSerialLen;
  call.args.data = argsSerial;

  size_t callSerialLen = call__get_packed_size(&call);
  uint8_t *callSerial = (uint8_t *)malloc(callSerialLen);
  if (callSerial == NULL) {
    error = 1;
    goto errCallMalloc;
  }

  call__pack(&call, callSerial);

  // This would normally be across the network, but luckily we
  // have the client and server in the same process. Magic!
  uint8_t *retSerial;
  size_t retSerialLen;
  if (handleCall(&retSerial, &retSerialLen, callSerial, callSerialLen)) {
    error = 1;
    goto errHandleCall;
  }

  // Deserialize/Unpack the return message
  Return *ret = return__unpack(NULL, retSerialLen, retSerial);
  if (ret == NULL) {
    error = 1;
    goto errRetUnpack;
  }

  // Deserialize/Unpack the return value of the invert call
  InvertReturnValue *value = invert_return_value__unpack(NULL, ret->value.len, ret->value.data);
  if (value == NULL) {
    error = 1;
    goto errValueUnpack;
  }

  if (ret->success) {
    *notv = value->notv;
    printf("invert(%d) returned %d\n", v, *notv);
  } else {
    error = 1;
    printf("invert(%d) RPC failed!\n", v);
  }

  // Cleanup
  invert_return_value__free_unpacked(value, NULL);
errValueUnpack:
  return__free_unpacked(ret, NULL);
errRetUnpack:
  free(retSerial);
errHandleCall:
  free(callSerial);
errCallMalloc:
  free(argsSerial);

  return error;
}

int main(int argc, const char *argv[])
{
  (void)argc;
  (void)argv;

  bool v = true;
  bool notv;
  callInvert(&notv, v);

  v = false;
  callInvert(&notv, v);

  return 0;
}
