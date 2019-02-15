/*
* Copyright (c) 2018 Atmosphère-NX
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public License,
* version 2, as published by the Free Software Foundation.
*
* This program is distributed in the hope it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <switch.h>
#include <stratosphere.hpp>
#include "hid_mitm_iappletresource.hpp"

enum HidCmd : u32
{
    HidCmd_CreateAppletResource = 0,
    HidCmd_ReloadHidMitmConfig = 65000
};

class HidMitmService : public IMitmServiceObject
{
  public:
    HidMitmService(std::shared_ptr<Service> s, u64 pid) : IMitmServiceObject(s, pid)
    {
        /* ... */
    }

    static bool ShouldMitm(u64 pid, u64 tid)
    {
        if(tid < 0x0100000000001000UL) // Don't mitm sysmodules
            return false;
        
        if(tid == 0x010000000000100CUL) // Don't mitm overlaydisp since that likes to take the focus and we want to respect that without having to keep multiple sharedmems around.
            return false;
        return true;
    }

    static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);

  protected:
    /* Overridden commands. */
    virtual Result CreateAppletResource(Out<std::shared_ptr<IAppletResourceMitmService>> out, PidDescriptor pid, u64 arid) final;
    virtual Result ReloadConfig() final;

  public:
    DEFINE_SERVICE_DISPATCH_TABLE{
        MakeServiceCommandMeta<HidCmd_CreateAppletResource, &HidMitmService::CreateAppletResource>(),
        MakeServiceCommandMeta<HidCmd_ReloadHidMitmConfig, &HidMitmService::ReloadConfig>(),
    };
};
