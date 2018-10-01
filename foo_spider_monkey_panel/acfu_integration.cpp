#include "stdafx.h"

#include <utils/json.hpp>
#include <acfu_github.h>

#include <acfu-sdk/utils/common.h>


namespace smp::acfu
{

class SmpSource
    : public ::acfu::source
    , public smp::acfu::github_conf
{
public:
    static pfc::string8 FetchVersion()
    {
        pfc::string8 version = "0.0.0";

        componentversion::ptr cv;
        ::acfu::for_each_service<componentversion>( [&]( auto& ptr ) 
        {
            pfc::string8 file_name;
            ptr->get_file_name( file_name );
            if ( file_name.equals( componentFileName_ ) )
            {
                cv = ptr;
            }
        } );
        if ( cv.is_empty() )
        {
            return version;
        }

        cv->get_component_version( version );
        return version;
    }
    virtual GUID get_guid()
    {
        return g_guid_acfu_source;
    }
    virtual void get_info( file_info& info )
    {
        if ( !isVersionFetched_ )
        {
            installedVersion_ = FetchVersion();
            isVersionFetched_ = true;
        }

        info.meta_set( "version", installedVersion_ );
        info.meta_set( "name", "Spider Monkey Panel" );
        info.meta_set( "module", componentFileName_ );
    }
    virtual bool is_newer( const file_info& info )
    {        
        if ( !info.meta_get( "version", 0 ) )
        {
            return false;
        }

        pfc::string8 available = info.meta_get( "version", 0 );
        if ( available.has_prefix( "v" ) )
        {
            available.set_string_nc( available.c_str() + 1, available.length() - 1 );
        }

        // We are using semantic versioning, so lexicographical comparison is fine
        return available > installedVersion_;
    }
    virtual ::acfu::request::ptr create_request()
    {
        return new service_impl_t<smp::acfu::github_latest_release<SmpSource>>();
    }
    static const char* get_owner()
    {
        return "TheQwertiest";
    }
    static const char* get_repo()
    {
        return componentFileName_;
    }

private:
    static constexpr char componentFileName_[] = "foo_spider_monkey_panel";
    bool isVersionFetched_ = false;
    pfc::string8 installedVersion_;
};
static service_factory_single_t<SmpSource> g_smpSource;

} // namespace smp::acfu