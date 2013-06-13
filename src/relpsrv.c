/* The relp server.
 *
 * Copyright 2008-2013 by Rainer Gerhards and Adiscon GmbH.
 *
 * This file is part of librelp.
 *
 * Librelp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Librelp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Librelp.  If not, see <http://www.gnu.org/licenses/>.
 *
 * A copy of the GPL can be found in the file "COPYING" in this distribution.
 *
 * If the terms of the GPL are unsuitable for your needs, you may obtain
 * a commercial license from Adiscon. Contact sales@adiscon.com for further
 * details.
 *
 * ALL CONTRIBUTORS PLEASE NOTE that by sending contributions, you assign
 * your copyright to Adiscon GmbH, Germany. This is necessary to permit the
 * dual-licensing set forth here. Our apologies for this inconvenience, but
 * we sincerely believe that the dual-licensing model helps us provide great
 * free software while at the same time obtaining some funding for further
 * development.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>
#include <sys/socket.h>
#include "relp.h"
#include "relpsrv.h"
#include "tcp.h"



/** Construct a RELP srv instance
 * This is the first thing that a caller must do before calling any
 * RELP function. The relp srv must only destructed after all RELP
 * operations have been finished.
 */
relpRetVal
relpSrvConstruct(relpSrv_t **ppThis, relpEngine_t *pEngine)
{
	relpSrv_t *pThis;

	ENTER_RELPFUNC;
	assert(ppThis != NULL);
	if((pThis = calloc(1, sizeof(relpSrv_t))) == NULL) {
		ABORT_FINALIZE(RELP_RET_OUT_OF_MEMORY);
	}

	RELP_CORE_CONSTRUCTOR(pThis, Srv);
	pThis->pEngine = pEngine;
	pThis->stateCmdSyslog = pEngine->stateCmdSyslog;
	pThis->ai_family = PF_UNSPEC;
	pThis->dhBits = DEFAULT_DH_BITS;
	pThis->pristring = NULL;
	pThis->caCertFile = NULL;
	pThis->ownCertFile = NULL;
	pThis->privKey = NULL;

	*ppThis = pThis;

finalize_it:
	LEAVE_RELPFUNC;
}


/** Destruct a RELP srv instance
 */
relpRetVal
relpSrvDestruct(relpSrv_t **ppThis)
{
	relpSrv_t *pThis;

	ENTER_RELPFUNC;
	assert(ppThis != NULL);
	pThis = *ppThis;
	RELPOBJ_assert(pThis, Srv);

	if(pThis->pTcp != NULL)
		relpTcpDestruct(&pThis->pTcp);

	if(pThis->pLstnPort != NULL)
		free(pThis->pLstnPort);

	free(pThis->pristring);
	free(pThis->caCertFile);
	free(pThis->ownCertFile);
	free(pThis->privKey);
	/* done with de-init work, now free srv object itself */
	free(pThis);
	*ppThis = NULL;

	LEAVE_RELPFUNC;
}


/* set the user pointer inside the RELP server. Whatever value the user
 * provides is accepted.
 * rgerhards, 2008-07-08
 */
relpRetVal
relpSrvSetUsrPtr(relpSrv_t *pThis, void *pUsr)
{
	ENTER_RELPFUNC;
	RELPOBJ_assert(pThis, Srv);
	pThis->pUsr = pUsr;
	LEAVE_RELPFUNC;
}


/* set the listen port inside the relp server. If NULL is provided, the default port
 * is used. The provided string is always copied, it is the caller's duty to
 * free the passed-in string.
 * rgerhards, 2008-03-17
 */
relpRetVal
relpSrvSetLstnPort(relpSrv_t *pThis, unsigned char *pLstnPort)
{
	ENTER_RELPFUNC;
	RELPOBJ_assert(pThis, Srv);

	/* first free old value */
	if(pThis->pLstnPort != NULL)
		free(pThis->pLstnPort);
	pThis->pLstnPort = NULL;

	if(pLstnPort != NULL) {
		if((pThis->pLstnPort = (unsigned char*) strdup((char*)pLstnPort)) == NULL)
			ABORT_FINALIZE(RELP_RET_OUT_OF_MEMORY);
	}
		
finalize_it:
	LEAVE_RELPFUNC;
}


/* set the IPv4/v6 type to be used. Default is both (PF_UNSPEC)
 * rgerhards, 2013-03-15
 */
relpRetVal
relpSrvSetFamily(relpSrv_t *pThis, int ai_family)
{
	ENTER_RELPFUNC;
	RELPOBJ_assert(pThis, Srv);
	pThis->ai_family = ai_family;
	LEAVE_RELPFUNC;
}

/* set the GnuTLS priority string. Providing NULL does re-set
 * any previously set string. -- rgerhards, 2013-06-12
 */
relpRetVal
relpSrvSetGnuTLSPriString(relpSrv_t *pThis, char *pristr)
{
	ENTER_RELPFUNC;
	RELPOBJ_assert(pThis, Srv);
	free(pThis->pristring);
	if(pristr == NULL) {
		pThis->pristring = NULL;
	} else {
		if((pThis->pristring = strdup(pristr)) == NULL)
			ABORT_FINALIZE(RELP_RET_OUT_OF_MEMORY);
	}
finalize_it:
	LEAVE_RELPFUNC;
}

relpRetVal
relpSrvSetCACert(relpSrv_t *pThis, char *cert)
{
	ENTER_RELPFUNC;
	RELPOBJ_assert(pThis, Srv);
	free(pThis->caCertFile);
	if(cert == NULL) {
		pThis->caCertFile = NULL;
	} else {
		if((pThis->caCertFile = strdup(cert)) == NULL)
			ABORT_FINALIZE(RELP_RET_OUT_OF_MEMORY);
	}
finalize_it:
	LEAVE_RELPFUNC;
}
relpRetVal
relpSrvSetOwnCert(relpSrv_t *pThis, char *cert)
{
	ENTER_RELPFUNC;
	RELPOBJ_assert(pThis, Srv);
	free(pThis->ownCertFile);
	if(cert == NULL) {
		pThis->ownCertFile = NULL;
	} else {
		if((pThis->ownCertFile = strdup(cert)) == NULL)
			ABORT_FINALIZE(RELP_RET_OUT_OF_MEMORY);
	}
finalize_it:
	LEAVE_RELPFUNC;
}
relpRetVal
relpSrvSetPrivKey(relpSrv_t *pThis, char *cert)
{
	ENTER_RELPFUNC;
	RELPOBJ_assert(pThis, Srv);
	free(pThis->privKey);
	if(cert == NULL) {
		pThis->privKey = NULL;
	} else {
		if((pThis->privKey = strdup(cert)) == NULL)
			ABORT_FINALIZE(RELP_RET_OUT_OF_MEMORY);
	}
finalize_it:
	LEAVE_RELPFUNC;
}

void
relpSrvSetDHBits(relpSrv_t *pThis, int bits)
{
	pThis->dhBits = bits;
}
void
relpSrvEnableTLS(relpSrv_t *pThis)
{
	pThis->bEnableTLS = 1;
}
void
relpSrvEnableTLSZip(relpSrv_t *pThis)
{
	pThis->bEnableTLSZip = 1;
}


/* start a relp server - the server object must have all properties set
 * rgerhards, 2008-03-17
 */
relpRetVal
relpSrvRun(relpSrv_t *pThis)
{
	relpTcp_t *pTcp;

	ENTER_RELPFUNC;
	RELPOBJ_assert(pThis, Srv);

	CHKRet(relpTcpConstruct(&pTcp, pThis->pEngine, RELP_SRV_CONN));
	if(pThis->bEnableTLS) {
		relpTcpEnableTLS(pTcp);
		if(pThis->bEnableTLSZip) {
			relpTcpEnableTLSZip(pTcp);
		}
		relpTcpSetDHBits(pTcp, pThis->dhBits);
		relpTcpSetGnuTLSPriString(pTcp, pThis->pristring);
		relpTcpSetCACert(pTcp, pThis->caCertFile);
		relpTcpSetOwnCert(pTcp, pThis->ownCertFile);
		relpTcpSetPrivKey(pTcp, pThis->privKey);
	}
	CHKRet(relpTcpLstnInit(pTcp, (pThis->pLstnPort == NULL) ? (unsigned char*) RELP_DFLT_PORT : pThis->pLstnPort, pThis->ai_family));
		
	pThis->pTcp = pTcp;

finalize_it:
	if(iRet != RELP_RET_OK) {
		if(pThis->pTcp != NULL)
			relpTcpDestruct(&pTcp);
	}

	LEAVE_RELPFUNC;
}


/* Enable or disable a command. Note that a command can not be enabled once
 * it has been set to forbidden! There will be no error return state in this
 * case.
 * rgerhards, 2008-03-27
 */
relpRetVal
relpSrvSetEnableCmd(relpSrv_t *pThis, unsigned char *pszCmd, relpCmdEnaState_t stateCmd)
{
	ENTER_RELPFUNC;
	RELPOBJ_assert(pThis, Srv);
	assert(pszCmd != NULL);

pThis->pEngine->dbgprint("SRV SetEnableCmd in syslog cmd state: %d\n", pThis->stateCmdSyslog);
	if(!strcmp((char*)pszCmd, "syslog")) {
		if(pThis->stateCmdSyslog != eRelpCmdState_Forbidden)
			pThis->stateCmdSyslog = stateCmd;
	} else {
		pThis->pEngine->dbgprint("tried to set unknown command '%s' to %d\n", pszCmd, stateCmd);
		ABORT_FINALIZE(RELP_RET_UNKNOWN_CMD);
	}

finalize_it:
pThis->pEngine->dbgprint("SRV SetEnableCmd out syslog cmd state: %d, iRet %d\n", pThis->stateCmdSyslog, iRet);
	LEAVE_RELPFUNC;
}
