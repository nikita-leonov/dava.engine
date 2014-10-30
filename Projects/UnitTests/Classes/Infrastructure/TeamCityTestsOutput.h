/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#ifndef __DAVAENGINE_TEAMCITY_TEST_OUTPUT_H__
#define __DAVAENGINE_TEAMCITY_TEST_OUTPUT_H__

/**
	\defgroup utils Utilities
 */

#include "TeamcityOutput/TeamcityOutput.h"

#include "SFML/Network/IpAddress.hpp"
#include "SFML/Network/TcpSocket.hpp"

namespace DAVA 
{

   
class TeamcityTestOutput: public TeamcityOutput
{
public:
	TeamcityTestOutput():connected(false){}

    virtual void Output(Logger::eLogLevel ll, const char8* text) const;

	static String FormatTestStarted(const String& testName);
	static String FormatTestFinished(const String& testName);
	static String FormatTestFailed(const String& testName, const String& condition, const String& errMsg);

	void connect(const String& host, unsigned int port);
	void sendTestResult(const String& testResult) const;
	void disconnect();

private:

	void TestOutput(const String& data) const;

	mutable sf::TcpSocket socket;
	mutable bool connected;
};


};

#endif // __DAVAENGINE_TEAMCITY_TEST_OUTPUT_H__

