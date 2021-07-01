/*******************************************************************************
 * Copyright (c) 2021, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the EtherNet/IP Security object
 * @author Markus Pešek <markus.pesek@tuwien.ac.at>
 *
 *  EtherNet/IP Security object
 *  ===================
 *
 *  This module implements the EtherNet/IP Security object.
 *
 *  Implemented Attributes
 *  ----------------------
 *  - Attribute  1: State
 *  - Attribute  2: Capability Flags
 *  - Attribute  3: Available Cipher Suites
 *  - Attribute  4: Allowed Cipher Suites
 *  - Attribute  5: Pre-Shared Keys
 *  - Attribute  6: Active Device Certificates
 *  - Attribute  7: Trusted Authorities
 *  - Attribute  8: Certificate Revocation List
 *  - Attribute  9: Verify Client Certificate
 *  - Attribute  10: Send Certificate Chain
 *  - Attribute  11: Check Expiration
 *  - Attribute  12: Trusted Identities
 *  - Attribute  13: Pull Model Enable
 *  - Attribute  14: Pull Model Status
 *  - Attribute  15: DTLS Timeout
 *  - Attribute  16: UDP-only Policy
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributesAll
 *  - Reset
 *  - GetAttributeSingle
 *  - SetAttributeSingle
 *  - Begin_Config
 *  - Kick_Timer
 *  - Apply_Config
 *  - Abort_Config
 */

/* ********************************************************************
 * include files
 */
#include "string.h"
#include <stdio.h>

#include "ethernetipsecurity.h"

#include "cipcommon.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"

/* ********************************************************************
 * defines
 */
/** The implemented class revision is 5 */
#define ETHERNET_IP_SECURITY_OBJECT_REVISION 5

/* ********************************************************************
 * global public variables
 */
/**< definition of EtherNet/IP Security object instance 1 data */

//TODO: remove section ######################################################
CipEpath dummy_CMO_Paths[1] = {  // dummy Certificate Management object path
		{
				2, /* PathSize in 16 Bit chunks */
				0x5F, /* Class Code */
				0x01, /* Instance # */
				0 /* Attribute # */
} };

 EIPSecurityObjectPathList const dummy_CMO_path_list = {
		  1,
		  dummy_CMO_Paths
 };
 // #########################################################################

EIPSecurityObject g_eip_security = { //TODO: add object configuration
		.state = kEIPSecurityObjectStateFactoryDefaultConfiguration,     /** Attribute #1 */
		.active_device_certificates = dummy_CMO_path_list,               /** Attribute #6 */
		.pre_shared_keys.number_of_pre_shared_keys = 0,                  /** Attribute #5 */
		.pull_model_enabled = true,  //default: true                     /** Attribute #13 */
		.pull_model_enabled = 0x0000,                                    /** Attribute #14 */
		.dtls_timeout = 0x0C //default: 12 seconds                       /** Attribute #15 */
};
//  .capability_flags =0,                           /** Attribute #2 */
//  .available_cipher_suites = 0,                   /** Attribute #3 */
//  .allowed_cipher_suites,                         /** Attribute #4 */
//  .trusted_authorities,                           /** Attribute #7 */
//  .certificate_revocation_list,                   /** Attribute #8 */
//  .verify_client_certificate,                     /** Attribute #9 */
//  .send_certificate_chain,                        /** Attribute #10 */
//  .check_expiration,                              /** Attribute #11 */
//  .trusted_identities,                            /** Attribute #12 */
//  .udp_only_policy                                /** Attribute #16 */

/* ********************************************************************
 * public functions
 */

/** @brief EtherNet/IP Security Object Reset service
 *
 * Return this EtherNet/IP Security Object Instance to the
 * Factory Default Configuration State.
 * See Vol.8 Section 5-4.5.1
 */
EipStatus EIPSecurityObjectReset(CipInstance *RESTRICT const instance,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response,
		const struct sockaddr *originator_address,
		const int encapsulation_session) {

	message_router_response->general_status = kCipErrorPrivilegeViolation; //TODO: check error status
	message_router_response->size_of_additional_status = 0;
	InitializeENIPMessage(&message_router_response->message);
	message_router_response->reply_service = (0x80
			| message_router_request->service);

	// TODO: if state is factory-default: do nothing
	//use: kEIPSecurityObjectStateFactoryDefaultConfiguration

	CipBool enable_pull_model = true; //optional request parameter (true if not received)

	enable_pull_model = GetBoolFromMessage(&message_router_request->data); //TODO: check if data is sent

	OPENER_TRACE_INFO("DEBUG: enable_pull_model %d\n", enable_pull_model); //TODO: remove

	//TODO: set attribute 13
	CipAttributeStruct *attribute = GetCipAttribute(instance, 13); //attribute 13: pull model enable

	attribute->data = enable_pull_model; //TODO: check this

	/* TODO:  Reset settable attributes of each existing EtherNet/IP Security Object to factory default*/
	//TODO: set attributes: 4 - 12, 15,16
//		for (CipInstance *ins = instance->cip_class->instances; ins; ins =
//				ins->next) /* follow the list*/
//				{
//			OPENER_TRACE_INFO("DEBUG: ethernetip_security_object instance  %d\n", ins->instance_number);
//		}
	return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Begin_Config service
 *
 * Causes the object to transition to the Configuration In Progress state.
 * See Vol.8 Section 5-4.7.1
 */
EipStatus EIPSecurityObjectBeginConfig(CipInstance *RESTRICT const instance,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response,
		const struct sockaddr *originator_address,
		const int encapsulation_session) {

	message_router_response->general_status = kCipErrorSuccess;
	message_router_response->size_of_additional_status = 0;
	InitializeENIPMessage(&message_router_response->message);
	message_router_response->reply_service = (0x80
			| message_router_request->service);

	if ((kEIPSecurityObjectStatePullModelDisabled
					|| kEIPSecurityObjectStatePullModelCompleted
					|| kEIPSecurityObjectStateConfigured) == g_eip_security.state) {
		message_router_response->general_status = kCipErrorObjectStateConflict;
	} else {
		g_eip_security.state = kEIPSecurityObjectStateConfigurationInProgress;
	}

	return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Kick_Timer service
 *
 * Causes the object to reset the configuration session timer.
 * See Vol.8 Section 5-4.7.2
 */
EipStatus EIPSecurityObjectKickTimer(CipInstance *RESTRICT const instance,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response,
		const struct sockaddr *originator_address,
		const int encapsulation_session) {

	message_router_response->general_status = kCipErrorObjectStateConflict;
	message_router_response->size_of_additional_status = 0;
	InitializeENIPMessage(&message_router_response->message);
	message_router_response->reply_service = (0x80
			| message_router_request->service);

	if (kEIPSecurityObjectStateConfigurationInProgress == g_eip_security.state) {
			message_router_response->general_status = kCipErrorSuccess;
	}

	return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Apply_Config service
 *
 * Applies the configuration and places the object in the Configured state.
 * See Vol.8 Section 5-4.7.3
 */
EipStatus EIPSecurityObjectApplyConfig(CipInstance *RESTRICT const instance,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response,
		const struct sockaddr *originator_address,
		const int encapsulation_session) {

	message_router_response->general_status = kCipErrorObjectStateConflict;
	message_router_response->size_of_additional_status = 0;
	InitializeENIPMessage(&message_router_response->message);
	message_router_response->reply_service = (0x80
			| message_router_request->service);

	//TODO: implement service

	return kEipStatusOk;
}

/** @brief EtherNet/IP Security Object Abort_Config service
 *
 * Abort the current configuration and discard pending changes.
 * See Vol.8 Section 5-4.7.4
 */
EipStatus EIPSecurityObjectAbortConfig(CipInstance *RESTRICT const instance,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response,
		const struct sockaddr *originator_address,
		const int encapsulation_session) {

	message_router_response->general_status = kCipErrorObjectStateConflict;
	message_router_response->size_of_additional_status = 0;
	InitializeENIPMessage(&message_router_response->message);
	message_router_response->reply_service = (0x80
			| message_router_request->service);

	//TODO: implement service

	return kEipStatusOk;
}

void FinalizeMessage(CipUsint general_status,
                     CipMessageRouterRequest *message_router_request,
                     CipMessageRouterResponse *message_router_response) {
  message_router_response->general_status = general_status;
  message_router_response->size_of_additional_status = 0;
  InitializeENIPMessage(&message_router_response->message);
  message_router_response->reply_service = (0x80 | message_router_request->service);
}

EipStatus SetAttributeSingleEIPSecurityObject(  //TODO: remove - use SetAttributeSingle
    CipInstance *instance,
    CipMessageRouterRequest *message_router_request,
    CipMessageRouterResponse *message_router_response,
    const struct sockaddr *originator_address,
    const int encapsulation_session) {

  EipUint16 attribute_number = message_router_request->request_path.attribute_number;
  CipAttributeStruct *attribute = GetCipAttribute(instance, attribute_number);

  /* we don't have this attribute */
  if (NULL == attribute) {
    FinalizeMessage(kCipErrorAttributeNotSupported,
                    message_router_request,
                    message_router_response);
    return kEipStatusOkSend;
  }

  uint8_t set_bit_mask = (instance->cip_class->set_bit_mask[
      CalculateIndex(attribute_number)
  ]);
  if (set_bit_mask & (1 << ((attribute_number) % 8))) {
    FinalizeMessage(kCipErrorAttributeNotSetable,
                    message_router_request,
                    message_router_response);
    return kEipStatusOkSend;
  }

  if (attribute->data == NULL) {
    FinalizeMessage(kCipErrorNotEnoughData,
                    message_router_request,
                    message_router_response);
    return kEipStatusOkSend;
  }

  // Execute PreSetCallback iff available
  if ((attribute->attribute_flags & kPreSetFunc) && instance->cip_class->PreSetCallback ) {
    instance->cip_class->PreSetCallback(instance, attribute, message_router_request->service);
  }

  CipDword *data = (CipDword *) attribute->data;

  OPENER_TRACE_INFO(" setAttribute %d\n", attribute_number);

  switch (attribute_number) {
    case 5: { /** Attribute 5: Pre-Shared Keys **/
      CipUsint number_of_psk = GetUsintFromMessage(&(message_router_request->data));
      EIPSecurityObjectPreSharedKeys *pre_shared_keys = data;

      // At present, a maximum of 1 PSK may be configured
      if (number_of_psk > 1) {
        message_router_response->general_status = kCipErrorInvalidAttributeValue;
        break;
      }

      if (number_of_psk == 1) {
        EIPSecurityObjectPreSharedKey *psk_structure =
            CipCalloc(number_of_psk, sizeof(EIPSecurityObjectPreSharedKey));

        psk_structure->psk_identity_size = GetUsintFromMessage(&(message_router_request->data));

        if (psk_structure->psk_identity_size <= SIZE_MAX_PSK_IDENTITY){
          CipOctet *psk_identity = CipCalloc(psk_structure->psk_identity_size, sizeof(CipOctet));

          memcpy(psk_identity,
                 message_router_request->data,
                 psk_structure->psk_identity_size);
          message_router_request->data += psk_structure->psk_identity_size;
//          for (int i=0; i<psk_structure->psk_identity_size; i++) {
//            psk_identity[i] = GetByteFromMessage(&(message_router_request->data));
//          }

          psk_structure->psk_identity = psk_identity;
          psk_structure->psk_size = GetUsintFromMessage(&(message_router_request->data));

          if(psk_structure->psk_size <= SIZE_MAX_PSK) {
            CipOctet *psk = CipCalloc(psk_structure->psk_size, sizeof(CipOctet));

            memcpy(psk, message_router_request->data, psk_structure->psk_size);
//            for (int i=0; i<psk_structure->psk_size; i++) {
//              psk[i] = GetByteFromMessage(&(message_router_request->data));
//            }

            psk_structure->psk = psk;
            //TODO: Cleanup existing PSKs
            pre_shared_keys->pre_shared_keys = psk_structure;
            message_router_response->general_status = kCipErrorSuccess;
          } else {
            if (psk_identity != NULL){
              CipFree(psk_identity);
              psk_structure->psk_identity = NULL;
            }
            if (psk_structure != NULL) {
              CipFree(psk_structure);
            }
            message_router_response->general_status = kCipErrorInvalidAttributeValue;
          }
        } else {
          if (psk_structure != NULL) {
            CipFree(psk_structure);
          }
          message_router_response->general_status = kCipErrorInvalidAttributeValue;
        }
      } else {
        //TODO: Cleanup existing PSKs
        pre_shared_keys->number_of_pre_shared_keys = number_of_psk; //0
        pre_shared_keys->pre_shared_keys=NULL;
        message_router_response->general_status = kCipErrorSuccess;
      }
    }
      break;

    default:
      message_router_response->general_status = kCipErrorAttributeNotSetable;
      break;
  } //end of switch

  message_router_response->size_of_additional_status = 0;
  message_router_response->reply_service = (0x80 | message_router_request->service);

  return kEipStatusOkSend;
}

void EncodeEIPSecurityObjectCipherSuiteId(const void *const data,
                                          ENIPMessage *const outgoing_message)
{
  EIPSecurityObjectCipherSuiteId *cipher_suite_id =
      (EIPSecurityObjectCipherSuiteId *) data;

  EncodeCipUsint(&(cipher_suite_id->iana_first_byte), outgoing_message);
  EncodeCipUsint(&(cipher_suite_id->iana_second_byte), outgoing_message);
}

int DecodeEIPSecurityObjectCipherSuites(
		EIPSecurityObjectCipherSuites *const data,
		const CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response) {

	int number_of_decoded_bytes = -1;

	CipUsint number_of_cipher_suites = GetUsintFromMessage(
			&(message_router_request->data));
	number_of_decoded_bytes = sizeof(number_of_cipher_suites);
	CipFree(data->cipher_suite_ids);

	if (number_of_cipher_suites > 0) {
		EIPSecurityObjectCipherSuiteId *cipher_suite_ids = CipCalloc(
				number_of_cipher_suites,
				sizeof(EIPSecurityObjectCipherSuiteId));

		memcpy(cipher_suite_ids, &(message_router_request->data),
				number_of_cipher_suites
						* sizeof(EIPSecurityObjectCipherSuiteId));

		number_of_decoded_bytes += number_of_cipher_suites
				* sizeof(EIPSecurityObjectCipherSuiteId);

		data->number_of_cipher_suites = number_of_cipher_suites;
		data->cipher_suite_ids = cipher_suite_ids;
	} else {
		data->cipher_suite_ids = NULL;
	}

	message_router_response->general_status = kCipErrorSuccess;
	return number_of_decoded_bytes;
}

void EncodeEIPSecurityObjectCipherSuites(const void *const data,
                                           ENIPMessage *const outgoing_message)
{
  EIPSecurityObjectCipherSuites *cipher_suites =
      (EIPSecurityObjectCipherSuites *) data;

  EncodeCipUsint(&(cipher_suites->number_of_cipher_suites), outgoing_message);

  for (int i=0; i<cipher_suites->number_of_cipher_suites; i++) {
    EncodeEIPSecurityObjectCipherSuiteId(&(cipher_suites->cipher_suite_ids[i]),
                                         outgoing_message);
  }
}

void EncodeEIPSecurityObjectPath(const CipEpath *const data,
		ENIPMessage *const outgoing_message) {
	AddSintToMessage(data->path_size, outgoing_message);
	if(0 != data->path_size){
		EncodeEPath((CipEpath*) data, outgoing_message);
	}
}

int DecodeEIPSecurityObjectPath(
		CipEpath *const epath,
		const CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response) {

	int number_of_decoded_bytes = -1;

	//TODO: implement function
	//number_of_decoded_bytes = DecodePaddedEPath(epath, &message_router_request->data);

	//TODO: update message_router_response->general_status
	return number_of_decoded_bytes;
}

void EncodeEIPSecurityObjectPathList(const void *const data,
                                       ENIPMessage *const outgoing_message)
{
  EIPSecurityObjectPathList *path_list = (EIPSecurityObjectPathList *) data;

  EncodeCipUsint(&(path_list->number_of_paths), outgoing_message);

  for (int i=0; i<path_list->number_of_paths; i++) {
    EncodeEIPSecurityObjectPath(&(path_list->paths[i]), outgoing_message);
  }
}

int DecodeEIPSecurityObjectPathList(
		EIPSecurityObjectPathList *const path_list,
		const CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response) {

	int number_of_decoded_bytes = -1;

	const EipUint8 **const cip_message = message_router_request->data;
	path_list->number_of_paths = GetUsintFromMessage(&cip_message);
	number_of_decoded_bytes = 1;

	//TODO: implement function
	// loop over Epaths in list

	//TODO: update message_router_response->general_status

	return number_of_decoded_bytes;
}

/**
 * When accessed via Get_Attributes_All or Get_Attribute_Single, the Size of
 * PSK element shall be 0, and 0 bytes of PSK value shall be returned.
 * This ensures that the PSK value cannot be read out of the device,
 * as it is a confidential piece of information.
 */
void EncodeEIPSecurityObjectPreSharedKeys(const void *const data,
                                         ENIPMessage *const outgoing_message) {
  AddSintToMessage(0, outgoing_message);
}

int DecodeEIPSecurityObjectPreSharedKeys(
		EIPSecurityObjectPreSharedKeys *const pre_shared_keys,
		const CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response) {

	int number_of_decoded_bytes = -1;

	//TODO: implement function

	//TODO: update message_router_response->general_status

	return number_of_decoded_bytes;
}

void EIPSecurityObjectInitializeClassSettings(CipClass *class) {

  CipClass *meta_class = class->class_instance.cip_class;

  InsertAttribute((CipInstance *) class,
                  1,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->revision,
                  kGetableSingleAndAll);  /* revision */
  InsertAttribute((CipInstance *) class,
                  2,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->number_of_instances,
                  kGetableSingleAndAll); /*  largest instance number */
  InsertAttribute((CipInstance *) class,
                  3,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->number_of_instances,
                  kGetableSingle); /* number of instances currently existing*/
  InsertAttribute((CipInstance *) class,
                  4,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &kCipUintZero,
                  kNotSetOrGetable); /* optional attribute list - default = 0 */
  InsertAttribute((CipInstance *) class,
                  5,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &kCipUintZero,
                  kNotSetOrGetable); /* optional service list - default = 0 */
  InsertAttribute((CipInstance *) class,
                  6,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &meta_class->highest_attribute_number,
                  kGetableSingleAndAll); /* max class attribute number*/
  InsertAttribute((CipInstance *) class,
                  7,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->highest_attribute_number,
                  kGetableSingleAndAll); /* max instance attribute number*/

  /* Add class services to the meta class */
  InsertService(meta_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll"
  );
  InsertService(meta_class,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle"
  );
}


EipStatus EIPSecurityInit(void) {
  CipClass *eip_security_object_class = NULL;
  CipInstance *eip_security_object_instance;

  eip_security_object_class =
      CreateCipClass(kEIPSecurityObjectClassCode,
                     0, /* # class attributes */
                     7, /* # highest class attribute number */
                     2, /* # class services */
                     16, /* # instance attributes */
                     16, /* # highest instance attribute number */
                     8, /* # instance services */
                     1, /* # instances*/
                     "EtherNet/IP Security Object",
                     ETHERNET_IP_SECURITY_OBJECT_REVISION, /* # class revision */
                     &EIPSecurityObjectInitializeClassSettings /* # function pointer for initialization */
      );

  if (NULL == eip_security_object_class) {
    /* Initialization failed */
    return kEipStatusError;
  }

  /* Bind attributes to the instance created above */
  eip_security_object_instance = GetCipInstance(eip_security_object_class, 1);

  InsertAttribute(eip_security_object_instance,
                  1,
                  kCipUsint,
                  EncodeCipUsint,
                  NULL,
                  &g_eip_security.state,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  2,
                  kCipWord,
                  EncodeCipDword,
                  NULL,
                  &g_eip_security.capability_flags,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  3,
                  kCipAny,
                  EncodeEIPSecurityObjectCipherSuites,
                  NULL,
                  &g_eip_security.available_cipher_suites,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  4,
                  kCipAny,
                  EncodeEIPSecurityObjectCipherSuites,
                  DecodeEIPSecurityObjectCipherSuites,
                  &g_eip_security.allowed_cipher_suites,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  5,
                  kCipAny,
                  EncodeEIPSecurityObjectPreSharedKeys,
                  DecodeEIPSecurityObjectPreSharedKeys, //TODO: implement decode function
                  &g_eip_security.pre_shared_keys,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  6,
                  kCipAny,
                  EncodeEIPSecurityObjectPathList,
                  DecodeEIPSecurityObjectPathList, //TODO: implement decode function
                  &g_eip_security.active_device_certificates,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  7,
                  kCipAny,
                  EncodeEIPSecurityObjectPathList,
                  DecodeEIPSecurityObjectPathList, //TODO: implement decode function
                  &g_eip_security.trusted_authorities,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  8,
                  kCipEpath,
                  EncodeEIPSecurityObjectPath,
                  DecodeEIPSecurityObjectPath, //TODO: implement decode function
                  &g_eip_security.certificate_revocation_list,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  9,
                  kCipBool,
                  EncodeCipBool,
                  DecodeCipBool,
                  &g_eip_security.verify_client_certificate,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  10,
                  kCipBool,
                  EncodeCipBool,
                  DecodeCipBool,
                  &g_eip_security.send_certificate_chain,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  11,
                  kCipBool,
                  EncodeCipBool,
                  DecodeCipBool,
                  &g_eip_security.check_expiration,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  12,
                  kCipAny,
                  EncodeEIPSecurityObjectPathList,
                  DecodeEIPSecurityObjectPathList, //TODO: implement decode function
                  &g_eip_security.trusted_identities,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  13,
                  kCipBool,
                  EncodeCipBool,
                  NULL,
                  &g_eip_security.pull_model_enabled,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  14,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  &g_eip_security.pull_model_status,
                  kGetableSingleAndAll
  );
  InsertAttribute(eip_security_object_instance,
                  15,
                  kCipUint,
                  EncodeCipUint,
                  DecodeCipUint, //TODO: implement DecodeDTLSTimeout (value check + error status)
                  &g_eip_security.dtls_timeout,
                  kSetAndGetAble
  );
  InsertAttribute(eip_security_object_instance,
                  16,
                  kCipUsint,
                  EncodeCipUsint,
                  DecodeCipUsint,
                  &g_eip_security.udp_only_policy,
                  kSetAndGetAble
  );

  /* Add services to the instance */
  InsertService(eip_security_object_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectResetServiceCode,
                &EIPSecurityObjectReset,
                "EIPSecurityObjectReset"
  );
  InsertService(eip_security_object_class,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle"
  );
  InsertService(eip_security_object_class,
                kSetAttributeSingle,
                &SetAttributeSingle,
                "SetAttributeSingle"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectBeginConfigServiceCode,
                &EIPSecurityObjectBeginConfig,
                "EIPSecurityObjectBeginConfig"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectKickTimerServiceCode,
                &EIPSecurityObjectKickTimer,
                "EIPSecurityObjectKickTimer"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectApplyConfigServiceCode,
                &EIPSecurityObjectApplyConfig,
                "EIPSecurityObjectApplyConfig"
  );
  InsertService(eip_security_object_class,
                kEIPSecurityObjectAbortConfigServiceCode,
                &EIPSecurityObjectAbortConfig,
                "EIPSecurityObjectAbortConfig"
  );

  return kEipStatusOk;
}
