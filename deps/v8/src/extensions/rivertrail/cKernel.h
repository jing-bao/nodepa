/*
 * Copyright (c) 2011, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "v8.h"
#include "cContext.h"
#include "opencl_compat.h"

//#include "include/xpcom/nsCOMPtr.h"
//#include "include/xpcom/nsCycleCollectionParticipant.h"

//#define DPO_KERNEL_CID_STR "cf99095d-391f-4ee8-bdea-7e50c90c6fb2"
//#define DPO_KERNEL_CID { 0xcf99095d, 0x391f, 0x4ee8, {0xbd, 0xea, 0x7e, 0x50, 0xc9, 0x0c, 0x6f, 0xb2}}

#ifndef CKERNEL_H
#define CKERNEL_H

class CKernel
{
public:
	CKernel();
	~CKernel();
	static v8::Handle<v8::Value> toV8Object(CKernel* cKernel);
	int InitKernel(cl_command_queue aCmdQueue, cl_kernel aKernel, cl_mem failureMem);
	
	bool GetNumberOfArgs(unsigned int &aNumberOfArgs);
	// Methods to supply arguments to a kernel.
	bool SetArgument( unsigned int number, CData *argument);
	bool SetScalarArgument(unsigned int number, const v8::Handle<v8::Value> & argument, 
		const v8::Handle<v8::Value> & isInteger, const v8::Handle<v8::Value> & highPrecision);

	// Methods to run a kernel
	bool Run(unsigned int rank, unsigned int *shape, unsigned int *tile, unsigned int *_retval);

private:	
	int numberOfArgs;
	//CContext* parent;
	cl_kernel kernel;
	cl_command_queue cmdQueue;
	cl_mem failureMem;
//  /* additional members */
	
};
#endif