/*
 * Copyright (c) 2008 
 *
 * Antoine Levitt (smeuuh@gmail.com)
 * Steven Herbst (herbst@mit.edu)
 *
 * Hypertriton, Inc. <http://hypertriton.com/>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <edacious/core/core_begin.h>

#include <edacious/generic/capacitor.h>
#include <edacious/generic/diode.h>
#include <edacious/generic/ground.h>
#include <edacious/generic/inductor.h>
#include <edacious/generic/led.h>
#include <edacious/generic/nmos.h>
#include <edacious/generic/npn.h>
#include <edacious/generic/pmos.h>
#include <edacious/generic/pnp.h>
#include <edacious/generic/resistor.h>
#include <edacious/generic/semiresistor.h>
#include <edacious/generic/spst.h>
#include <edacious/generic/spdt.h>

__BEGIN_DECLS
extern void *esGenericClasses[];
void ES_GenericInit(void);
void ES_GenericDestroy(void);
__END_DECLS

#include <edacious/core/core_close.h>
