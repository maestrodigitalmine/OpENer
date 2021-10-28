/*******************************************************************************
 * Copyright (c) 2021, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
/** @file
 * @brief Implements the Certificate Management object
 * @author Markus Pešek <markus.pesek@tuwien.ac.at>
 * @author Michael Satovich <michael.satovich@tuwien.ac.at>
 *
 *  Certificate Management object
 *  =============================
 *
 *  This module implements the Certificate Management object.
 *
 *  Implemented Class Attributes
 *  ----------------------------
 *  - Attribute  8: Capability Flags
 *  - Attribute  9: Certificate List
 *  - Attribute 10: Certificate Encoding Flag
 *
 *  Implemented Instance Attributes
 *  -------------------------------
 *  - Attribute  1: Name
 *  - Attribute  2: State
 *  - Attribute  3: Device Certificate
 *  - Attribute  4: CA Certificate
 *  - Attribute  5: Certificate Encoding
 *
 *  Implemented Services
 *  --------------------
 *  - GetAttributesAll
 *  - Create (class level)
 *  - Delete
 *  - GetAttributeSingle
 *  - SetAttributeSingle
 *  - Create_CSR
 *  - Verify_Certificate
 */

/* ********************************************************************
 * include files
 */
#include "string.h"
#include <stdio.h>
#include <string.h>

#include "cipcommon.h"
#include "endianconv.h"
#include "opener_api.h"
#include "trace.h"

#include "certificatemanagement.h"
#include "opener-security/CipSecurityObject/cipsecurity.h"

/* ********************************************************************
 * defines
 */
/** The implemented class revision is 1 */
#define CERTIFICATE_MANAGEMENT_OBJECT_REVISION 1

/**
 * declaration of (static) Certificate Management object instance 1 data
 */

const char instance_1_name[] = "Default Device Certificate";

CipShortString const name = {
		.length = strlen(instance_1_name),
		.string = (EipByte*)(&instance_1_name)
};

const Certificate device_certificate = {
    .certificate_status = kCertificateVerified
    // TODO: add path
};

const Certificate ca_certificate = {
    .certificate_status = kCertificateVerified
    // TODO: add path
};

CertificateManagementObject g_certificate_management = {
	.name = name,                                                               /*Attribute 1*/
	.state = kVerified,                    /*Attribute 2*/
	.device_certificate = device_certificate,                                   /*Attribute 3*/
	.ca_certificate = ca_certificate,                                           /*Attribute 4*/
	.certificate_encoding = kCertificateEncodingPEM /*Attribute 5*/
};

/** @brief Produce the data according to CIP encoding onto the message buffer.
 *
 * This function may be used in own services for sending object data back to the
 * requester (e.g., getAttributeSingle).
 *  @param certificate pointer to the certificate object to encode
 *  @param outgoing_message pointer to the message to be sent
 */
void EncodeCertificateManagementObjectCertificate(const Certificate *const certificate,
                                          ENIPMessage *const outgoing_message) {
    AddSintToMessage(certificate->certificate_status, outgoing_message);

    EncodeCipSecurityObjectPath(&(certificate->path), outgoing_message);
}

/** @brief Retrieve the given object instance EPATH according to
 * CIP encoding from the message buffer.
 *
 * This function may be used for writing certificate object data
 * received from the request message (e.g., setAttributeSingle).
 *  @param certificate pointer to certificate object to be written.
 *  @param message_router_request pointer to the request where the data should be taken from
 *  @param message_router_response pointer to the response where status should be set
 *  @return length of taken bytes
 *          -1 .. error
 */
int DecodeCertificateManagementObjectCertificate(
    Certificate *const certificate,
	CipMessageRouterRequest *const message_router_request,
	CipMessageRouterResponse *const message_router_response) {

	int number_of_decoded_bytes = -1;

	certificate->certificate_status = GetUsintFromMessage(
			&message_router_request->data);
	number_of_decoded_bytes = 1;

	//write EPATH to the file object instance
	number_of_decoded_bytes += DecodeCipSecurityObjectPath(
						&(certificate->path),
						message_router_request,
						message_router_response);

	OPENER_TRACE_INFO("Number_of_decoded bytes: %d\n", number_of_decoded_bytes);
	return number_of_decoded_bytes;
}

/** @brief Produce the data according to CIP encoding onto the message buffer.
 *
 * This function may be used in own services for sending object data back to the
 * requester (e.g., getAttributeSingle).
 *  @param data pointer to the certificate list to encode
 *  @param outgoing_message pointer to the message to be sent
 */
void EncodeCertificateManagementObjectCertificateList(const void *const data,
                                                      ENIPMessage *const outgoing_message) {
  CertificateList *cert_list = (CertificateList *) data;

  EncodeCipUsint(&(cert_list->number_of_certificates), outgoing_message);
  for (int i=0; i<cert_list->number_of_certificates; i++) {
      EncodeCertificateManagementObjectCertificate(&(cert_list->certificates[i]), outgoing_message);
    }
}

/** @brief Bind attribute values to a certificate management object instance
 *
 *  @param instance pointer to the object where attributes should be written
 *  @param cmo certificate management object containing data to be written
 */
void CertificateManagementObjectBindAttributes(CipInstance *instance,
		CertificateManagementObject *cmo) {

    InsertAttribute(instance,
                    1,
                    kCipShortString,
                    EncodeCipShortString,
                    NULL,
                    &cmo->name,
                    kGetableSingleAndAll
    );
    InsertAttribute(instance,
                    2,
                    kCipUsint,
                    EncodeCipUsint,
                    NULL,
                    &cmo->state,
                    kGetableSingleAndAll
    );
    InsertAttribute(instance,
                    3,
                    kCipAny,
                    EncodeCertificateManagementObjectCertificate,
                    DecodeCertificateManagementObjectCertificate,
                    &cmo->device_certificate,
                    kSetAndGetAble
    );
    InsertAttribute(instance,
                    4,
                    kCipAny,
                    EncodeCertificateManagementObjectCertificate,
                    DecodeCertificateManagementObjectCertificate,
                    &cmo->ca_certificate,
                    kSetAndGetAble
    );
    InsertAttribute(instance,
                    5,
                    kCipUsint,
                    EncodeCipUsint,
                    NULL,
                    &cmo->certificate_encoding,
                    kGetableSingleAndAll
    );
}


/** @brief Certificate Management Object Create service
 *
 * The Create service shall be used to create a dynamic instance.
 * See Vol.8 Section 5-5.5.1
 */
EipStatus CertificateManagementObjectCreate(
		CipInstance *RESTRICT const instance,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response,
		const struct sockaddr *originator_address,
		const int encapsulation_session) {

	message_router_response->general_status = kCipErrorSuccess;
	message_router_response->size_of_additional_status = 0;
	InitializeENIPMessage(&message_router_response->message);
	message_router_response->reply_service = (0x80
			| message_router_request->service);

	if (message_router_request->request_data_size > 0) {

		CipClass *certificate_management_object_class = GetCipClass(
				kCertificateManagementObjectClassCode);

		CipInstance *certificate_management_object_instance = AddCipInstances(
				certificate_management_object_class, 1); /* add 1 instance*/

		CertificateManagementObject *new_cmo =
				(CertificateManagementObject*) CipCalloc(1,
						sizeof(CertificateManagementObject));

		new_cmo->name.length = GetUsintFromMessage(
				&message_router_request->data);

		new_cmo->name.string = (CipByte*) CipCalloc(
				new_cmo->name.length, sizeof(CipByte));

          memcpy(new_cmo->name.string, message_router_request->data, new_cmo->name.length);
          CertificateManagementObjectBindAttributes(certificate_management_object_instance, new_cmo);

          new_cmo->state = kCreated;
          OPENER_TRACE_INFO("CMO instance number %d created\n",
                            certificate_management_object_instance->instance_number);
        } else {
          message_router_response->general_status = kCipErrorNotEnoughData;
        }

	return kEipStatusOk;
}

/** @brief Certificate Management Object Delete service
 *
 * The Delete service shall be used to delete dynamic instances
 * (static instances shall return status code 0x2D, Instance Not Deletable).
 * See Vol.8 Section 5-5.5.2
 */
EipStatus CertificateManagementObjectDelete(
		CipInstance *RESTRICT const instance,
		CipMessageRouterRequest *const message_router_request,
		CipMessageRouterResponse *const message_router_response,
		const struct sockaddr *originator_address,
		const int encapsulation_session) {
	//TODO: implement service
	message_router_response->general_status = kCipErrorInstanceNotDeletable;
	message_router_response->size_of_additional_status = 0;
	InitializeENIPMessage(&message_router_response->message);
	message_router_response->reply_service = (0x80
			| message_router_request->service);

	CipClass *const cmo_class = GetCipClass(
			kCertificateManagementObjectClassCode);

	if (instance->instance_number != 1) { //static instance 1 should not be deleted

		CipInstance *instances = cmo_class->instances;

		// update pointers in instance list
		instances = cmo_class->instances; /* pointer to first instance */
		if (instances->instance_number == instance->instance_number) { //if instance to delete is head
			cmo_class->instances = instances->next;
		} else {
			while (NULL != instances->next) /* as long as what next points to is not zero */
			{
				CipInstance *next_instance = instances->next;
				if (next_instance->instance_number
						== instance->instance_number) {
					instances->next = next_instance->next;
					break;
				}
				instances = instances->next;
			}
		}
		OPENER_TRACE_INFO("CMO instance number %d deleted\n",
						instance->instance_number);

		CipFree(instance); //delete instance
		//TODO: free all allocated elements of instance

		cmo_class->number_of_instances -= 1; /* update the total number of instances recorded by the class - Attr. 3 */

		//update largest instance number (class Attribute 2)
		instances = cmo_class->instances;
		while (NULL != instances->next) { //get last element - should be largest number
			instances = instances->next;
		}
		cmo_class->max_instance = instances->instance_number;

		message_router_response->general_status = kCipErrorSuccess;
	}

	return kEipStatusOk;
}

/** @brief Certificate Management Object Create CSR service
 *
 * The Create_CSR service creates a Certificate Signing Request,
 * suitable for submission to an enrollment server or certificate authority for signing.
 * See Vol.8 Section 5-5.7.1
 */
EipStatus CertificateManagementObjectCreateCSR(CipInstance *RESTRICT const instance,
                                            CipMessageRouterRequest *const message_router_request,
                                            CipMessageRouterResponse *const message_router_response,
                                            const struct sockaddr *originator_address,
                                            const int encapsulation_session) {
	//TODO: implement service

	return kEipStatusOk;
}

/** @brief Certificate Management Object Verify Certificate service
 *
 * The Verify_Certificate shall cause the object to verify the certificate
 * indicated by the service parameter. This service will set the appropriate status value
 * for the Certificate Status field of all certificates involved in the verification.
 * See Vol.8 Section 5-5.7.2
 */
EipStatus CertificateManagementObjectVerifyCertificate(CipInstance *RESTRICT const instance,
                                            CipMessageRouterRequest *const message_router_request,
                                            CipMessageRouterResponse *const message_router_response,
                                            const struct sockaddr *originator_address,
                                            const int encapsulation_session) {

	//TODO: implement service

	return kEipStatusOk;
}

void CertificateManagementObjectInitializeClassSettings(CertificateManagementObjectClass *class) {

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
                  (void *) &class->max_instance,
                  kGetableSingleAndAll); /*  largest instance number */
  InsertAttribute((CipInstance *) class,
                  3,
                  kCipUint,
                  EncodeCipUint,
                  NULL,
                  (void *) &class->number_of_instances,
                  kGetableSingleAndAll); /* number of instances currently existing*/
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
  InsertAttribute((CipInstance *) class,
                  8,
                  kCipDword,
                  EncodeCipDword,
                  NULL,
                  (void *) &class->capability_flags,
                  kGetableSingleAndAll); /* Certificate Management capabilities*/
  InsertAttribute((CipInstance *) class,
                  9,
                  kCipAny,
                  EncodeCertificateManagementObjectCertificateList,
                  NULL,
                  (void *) &class->certificate_list,
                  kGetableSingleAndAll); /* List of device certificates*/
  InsertAttribute((CipInstance *) class,
                  10,
                  kCipDword,
                  EncodeCipDword,
                  NULL,
                  (void *) &class->certificate_encodings_flag,
                  kGetableSingleAndAll); /* Certificate encodings supported*/

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
  InsertService(meta_class,
                kCreate,
                &CertificateManagementObjectCreate,
                "CertificateManagementObjectCreate"
  );
}

EipStatus CertificateManagementObjectInit(void) {
  CipClass *certificate_management_object_class = NULL;
  CipInstance *certificate_management_object_instance;

  certificate_management_object_class =
      CreateCipClass(kCertificateManagementObjectClassCode,
                     3, /* # class attributes */
                     10,/* # highest class attribute number */
                     3, /* # class services */
                     5, /* # instance attributes */
                     5, /* # highest instance attribute number */
                     6, /* # instance services */
                     1, /* # instances*/
                     "Certificate Management Object",
                     CERTIFICATE_MANAGEMENT_OBJECT_REVISION, /* # class revision */
                     &CertificateManagementObjectInitializeClassSettings /* # function pointer for initialization */
      );

  if (NULL == certificate_management_object_class) {
    /* Initialization failed */
    return kEipStatusError;
  }

  certificate_management_object_instance = GetCipInstance(certificate_management_object_class, 1);

  /* Bind attributes to the static instance number 1 (default certificates)*/
  CertificateManagementObjectBindAttributes(
      certificate_management_object_instance, &g_certificate_management);

  /* Add services to the instance */
  InsertService(certificate_management_object_class,
                kGetAttributeAll,
                &GetAttributeAll,
                "GetAttributeAll"
  );
  InsertService(certificate_management_object_class,
                kDelete,
                &CertificateManagementObjectDelete,
                "CertificateManagementObjectDelete"
  );
  InsertService(certificate_management_object_class,
                kGetAttributeSingle,
                &GetAttributeSingle,
                "GetAttributeSingle"
  );
  InsertService(certificate_management_object_class,
                kSetAttributeSingle,
                &SetAttributeSingle,
                "SetAttributeSingle"
  );
  InsertService(certificate_management_object_class,
                kCreateCSR,
                &CertificateManagementObjectCreateCSR,
                "CertificateManagementObjectCreateCSR"
  );
  InsertService(certificate_management_object_class,
                kVerifyCertificate,
                &CertificateManagementObjectVerifyCertificate,
                "CertificateManagementObjectVerifyCertificate"
  );

  return kEipStatusOk;
}