#ifndef MTL_LOGS_HPP_INCLUDED
#define MTL_LOGS_HPP_INCLUDED

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// if -pthread or -fopenmp provided only
#ifdef _REENTRANT
#   include <thread>
#   include <mutex>
#endif

#ifndef MTL_LOG_NAMESPACE
#   define MTL_LOG_NAMESPACE mlog
#endif

#ifndef DECLSPEC
#   if defined(__WIN32__) || defined(__WINRT__)
#       define DECLSPEC __declspec(dllexport)
#   else
#       if defined(__GNUC__) && __GNUC__ >= 4
#           define DECLSPEC __attribute__((__visibility__("default")))
#       elif defined(__GNUC__) && __GNUC__ >= 2
#           define DECLSPEC __declspec(dllexport)
#       else
#           define DECLSPEC
#       endif
#   endif
#endif


namespace MTL_LOG_NAMESPACE
{
#   define MTL_LOG_DECLARE(funcName) \
        template<typename... Args> DECLSPEC void funcName(const Args&... args);
    MTL_LOG_DECLARE(info)
    MTL_LOG_DECLARE(warning)
    MTL_LOG_DECLARE(error)
    MTL_LOG_DECLARE(fatal)
    MTL_LOG_DECLARE(debug)
    inline DECLSPEC void loadConfiguration(const std::string& fname);
#   undef MTL_LOG_DECLARE
    
    namespace __details
    {
        class _Logger;
#       define MTL_LOG_FRIEND(funcName) \
            template<typename... Args> friend void MTL_LOG_NAMESPACE::funcName(const Args&... args);
#       define __DETAILS_FRIENDSHIPS \
            MTL_LOG_FRIEND(info) \
            MTL_LOG_FRIEND(warning) \
            MTL_LOG_FRIEND(error) \
            MTL_LOG_FRIEND(fatal) \
            MTL_LOG_FRIEND(debug)
        
        class __Header final
        {
            public:
                __Header(void) = delete;
                __Header(const std::string& str) : pattern(str)
                {
                    this->build();
                }
                __Header& operator=(const std::string& str)
                {
                    this->pattern = str;
                    this->build();
                    return *this;
                }
                ~__Header(void) = default;
                void display(std::ostream& out, const std::string& type, const char *const color,
                             const char *const nocolor, bool colorEnabled)
                {
                    for(const std::pair<int, int>& p : this->chunks)
                    {
                        switch(p.first)
                        {
                            case MTL_LOG_NAMESPACE::__details::__Header::TAG_DATE_ID:
                            {
                                time_t  tt  = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                                std::tm ltm = *localtime(&tt);
                                this->printTwoDigits(out, ltm.tm_mon+1) << '/';
                                this->printTwoDigits(out, ltm.tm_mday)  << '/';
                                out << ltm.tm_year + 1900 << ' ';
                                this->printTwoDigits(out, ltm.tm_hour) << ':';
                                this->printTwoDigits(out, ltm.tm_min) << ':';
                                this->printTwoDigits(out, ltm.tm_sec);
                                break;
                            }
                            case MTL_LOG_NAMESPACE::__details::__Header::TAG_THREAD_ID:
#                               ifdef _REENTRANT
                                out << "0x" << std::hex << std::this_thread::get_id() << std::dec;
#                               endif
                                break;
                            case MTL_LOG_NAMESPACE::__details::__Header::TAG_TYPE_ID:
                                if (colorEnabled)
                                {
                                    out << color;
                                }
                                out << type;
                                if (colorEnabled)
                                {
                                    out << nocolor;
                                }
                                break;
                            default:
                                for(int i=p.first;i<p.second;++i)
                                {
                                    out << this->pattern.at(i);
                                }
                        }
                    }
                }
            private:
                static constexpr const int         TAG_DATE_ID    = -1;
                static constexpr const int         TAG_TYPE_ID    = -2;
                static constexpr const int         TAG_THREAD_ID  = -3;
                static constexpr const char *const TAG_TYPE       = "{TYPE}";
                static constexpr const char *const TAG_THREAD     = "{THREAD}";
                static constexpr const char *const TAG_DATE       = "{DATE}";
                std::string                      pattern;
                std::vector<std::pair<int, int>> chunks;
                inline std::ostream& printTwoDigits(std::ostream& out, int value)
                {
                    out << ((value < 10) ? "0" : "") << value;
                    return out;
                }
                void splitAndMerge(int begin, int end, int id)
                {
                    auto it=this->chunks.begin();
                    for(;it!=this->chunks.end();++it)
                    {
                        if (it->second > begin)
                        {
                            std::pair<int, int> mid(id, static_cast<int>(std::string::npos));
                            decltype(mid)       last(begin+end, it->second);
                            it->second = begin;
                            it = this->chunks.insert(std::next(it), last);
                            this->chunks.insert(it, mid);
                            break;
                        }
                    }
                }
#               define SPLIT_AND_MERGE(var, tag, id) \
                    if (var != std::string::npos)\
                    {\
                        this->splitAndMerge(static_cast<int>(var),\
                            static_cast<int>(std::string(MTL_LOG_NAMESPACE::__details::__Header::tag).length()), id);\
                    }
                void build(void)
                {
                    this->chunks.clear();
                    std::size_t typePos    = this->pattern.find(MTL_LOG_NAMESPACE::__details::__Header::TAG_TYPE);
                    std::size_t threadPos  = this->pattern.find(MTL_LOG_NAMESPACE::__details::__Header::TAG_THREAD);
                    std::size_t datePos    = this->pattern.find(MTL_LOG_NAMESPACE::__details::__Header::TAG_DATE);
                    this->chunks.push_back({0, static_cast<int>(this->pattern.size())});
                    SPLIT_AND_MERGE(typePos,   TAG_TYPE, TAG_TYPE_ID);
                    SPLIT_AND_MERGE(datePos,   TAG_DATE, TAG_DATE_ID);
                    SPLIT_AND_MERGE(threadPos, TAG_THREAD, TAG_THREAD_ID);
                }
#               undef SPLIT_AND_MERGE
        };
        template<typename T>
        class __Static_declarer
        {
            protected:
                static bool          ENABLE_LOG;
                static bool          ENABLE_COLOR;
                static bool          ENABLE_SPACING;
                static bool          ENABLE_DEBUG;
                static bool          ENABLE_INFO;
                static bool          ENABLE_ERROR;
                static bool          ENABLE_WARNING;
                static bool          ENABLE_FATAL;
                static std::ostream* OUT;
                static bool          ENABLE_ALPHA_BOOL;
                static MTL_LOG_NAMESPACE::__details::__Header FORMAT;
                
            private:
#               ifdef _REENTRANT
                static std::mutex    MUTEX;
#               endif
                static constexpr const char *const C_YELLOW = "\033[1;33m";
                static constexpr const char *const C_RED    = "\033[1;31m";
                static constexpr const char *const C_GREEN  = "\033[1;32m";
                static constexpr const char *const C_BLUE   = "\033[1;36m";
                static constexpr const char *const C_BLANK  = "\033[0m";
                friend class _Logger;
                __DETAILS_FRIENDSHIPS
                __Static_declarer(void) = delete;
        };
#       define STATIC_DECLARATION(type, name, value) \
            template<typename T> type __Static_declarer<T>::name = value;
        STATIC_DECLARATION(bool,          ENABLE_LOG,        true)
        STATIC_DECLARATION(bool,          ENABLE_COLOR,      false)
        STATIC_DECLARATION(bool,          ENABLE_SPACING,    true)
        STATIC_DECLARATION(bool,          ENABLE_DEBUG,      true)
        STATIC_DECLARATION(bool,          ENABLE_INFO,       true)
        STATIC_DECLARATION(bool,          ENABLE_ERROR,      true)
        STATIC_DECLARATION(bool,          ENABLE_WARNING,    true)
        STATIC_DECLARATION(bool,          ENABLE_FATAL,      true)
        STATIC_DECLARATION(std::ostream*, OUT,               &std::cout)
        STATIC_DECLARATION(bool,          ENABLE_ALPHA_BOOL, true)
#       ifdef _REENTRANT
        template<typename T> std::mutex __Static_declarer<T>::MUTEX;
#       endif
        STATIC_DECLARATION(MTL_LOG_NAMESPACE::__details::__Header, FORMAT, std::string("[{TYPE} {DATE} {THREAD}] :"))
#       undef STATIC_DECLARATION
    }
    
#   define MTL_LOG_GET_SET(type, name, option) \
        static inline void enable##name(type value) noexcept\
        {\
            MTL_LOG_NAMESPACE::Options::option = value;\
        }\
        static inline type is##name##Enabled(void) noexcept\
        {\
            return MTL_LOG_NAMESPACE::Options::option;\
        }
    class DECLSPEC Options final : public MTL_LOG_NAMESPACE::__details::__Static_declarer<MTL_LOG_NAMESPACE::Options>
    {
        public:
            Options(void) = delete;
            MTL_LOG_GET_SET(bool, Log,       ENABLE_LOG)
            MTL_LOG_GET_SET(bool, Color,     ENABLE_COLOR)
            MTL_LOG_GET_SET(bool, Spacing,   ENABLE_SPACING)
            MTL_LOG_GET_SET(bool, AlphaBool, ENABLE_ALPHA_BOOL)
            MTL_LOG_GET_SET(bool, Debug,     ENABLE_DEBUG)
            MTL_LOG_GET_SET(bool, Warning,   ENABLE_WARNING)
            MTL_LOG_GET_SET(bool, Error,     ENABLE_ERROR)
            MTL_LOG_GET_SET(bool, Fatal,     ENABLE_FATAL)
            MTL_LOG_GET_SET(bool, Info,      ENABLE_INFO)
            static void setOutputStream(std::ostream* out)
            {
                MTL_LOG_NAMESPACE::Options::OUT = out;
            }
            static void setFormat(const std::string& format)
            {
                MTL_LOG_NAMESPACE::Options::FORMAT = format;
            }
        
    };
    
#   undef MTL_LOG_GET_SET
    namespace __details
    {
        class _Logger final
        {
            private:
                _Logger(void) = delete;
                __DETAILS_FRIENDSHIPS
                
                static void _print_(void)
                {
                    *MTL_LOG_NAMESPACE::Options::OUT << std::endl;
                }
                
                template<typename Actual, typename... Args>
                static void _print_(const Actual& a, const Args&... args)
                {
                    if (MTL_LOG_NAMESPACE::Options::ENABLE_SPACING)
                    {
                        *MTL_LOG_NAMESPACE::Options::OUT << ' ';
                    }
                    *MTL_LOG_NAMESPACE::Options::OUT << a;
                    MTL_LOG_NAMESPACE::__details::_Logger::_print_(args...);
                }
                
                static void assertValidity(void)
                {
                    if (MTL_LOG_NAMESPACE::Options::OUT == nullptr)
                    {
                        throw std::ios_base::failure("OUT must not be nullptr !");
                    }
                    if (!MTL_LOG_NAMESPACE::Options::OUT->good())
                    {
                        throw std::ios_base::failure("OUT must be a \"good()\" std::ostream* !");
                    }
                }
                
                template<typename... Args>
                static void _start_print(const char *const tag, const char *const color, const Args&... args)
                {
#                   ifdef _REENTRANT
                    MTL_LOG_NAMESPACE::Options::MUTEX.lock();
#                   endif
                    MTL_LOG_NAMESPACE::__details::_Logger::assertValidity();
                    *MTL_LOG_NAMESPACE::Options::OUT << ((MTL_LOG_NAMESPACE::Options::isAlphaBoolEnabled()) ? std::boolalpha
                                                                                                            : std::noboolalpha);
                    MTL_LOG_NAMESPACE::Options::FORMAT.display(*MTL_LOG_NAMESPACE::Options::OUT,
                                                               tag, color,
                                                               MTL_LOG_NAMESPACE::Options::C_BLANK,
                                                               MTL_LOG_NAMESPACE::Options::isColorEnabled());
                    if (!MTL_LOG_NAMESPACE::Options::ENABLE_SPACING)
                    {
                        *MTL_LOG_NAMESPACE::Options::OUT << ' ';
                    }
                    MTL_LOG_NAMESPACE::__details::_Logger::_print_(args...);
#                   ifdef _REENTRANT
                    MTL_LOG_NAMESPACE::Options::MUTEX.unlock();
#                   endif
                }
            
        };
    }
#   define MTL_LOG_METHOD(funcName, type, color, option) \
        template<typename... Args>\
        DECLSPEC void funcName(const Args&... args)\
        {\
            if (MTL_LOG_NAMESPACE::Options::isLogEnabled() && \
                MTL_LOG_NAMESPACE::Options::is##option##Enabled())\
            {\
                MTL_LOG_NAMESPACE::__details::_Logger::_start_print(type,\
                                                                    MTL_LOG_NAMESPACE::Options::color,\
                                                                    args...);\
            }\
        }
    MTL_LOG_METHOD(info,    "INFO   ", C_GREEN,  Info)
    MTL_LOG_METHOD(warning, "WARNING", C_YELLOW, Warning)
    MTL_LOG_METHOD(error,   "ERROR  ", C_RED,    Error)
    MTL_LOG_METHOD(fatal,   "FATAL  ", C_RED,    Fatal)
    MTL_LOG_METHOD(debug,   "DEBUG  ", C_BLUE,   Debug)
#   undef MTL_LOG_METHOD
    
#   define DUMP_LINE(file, text) file << text << std::endl;
#   define READ_BOOL(varname) \
        config >> foo >> equal >> value;\
        MTL_LOG_NAMESPACE::Options::enable##varname(value);
    inline DECLSPEC void loadConfiguration(const std::string& fname)
    {
        std::ifstream config(fname);
        if (config.good())
        {
            std::string foo;
            char        equal;
            bool        value;
            READ_BOOL(Log);
            READ_BOOL(Color);
            READ_BOOL(Spacing);
            READ_BOOL(AlphaBool);
            READ_BOOL(Info);
            READ_BOOL(Warning);
            READ_BOOL(Error);
            READ_BOOL(Fatal);
            READ_BOOL(Debug);
            config >> foo >> equal;
            std::getline(config, foo);
            MTL_LOG_NAMESPACE::Options::setFormat(foo);
            config.close();
        }
        else
        {
            std::ofstream dump(fname);
            if (dump.good())
            {
                DUMP_LINE(dump, "ENABLE_LOG:bool        = 1");
                DUMP_LINE(dump, "ENABLE_COLOR:bool      = 0");
                DUMP_LINE(dump, "ENABLE_SPACING:bool    = 1");
                DUMP_LINE(dump, "ENABLE_ALPHA_BOOL:bool = 1");
                DUMP_LINE(dump, "ENABLE_INFO:bool       = 1");
                DUMP_LINE(dump, "ENABLE_WARNING:bool    = 1");
                DUMP_LINE(dump, "ENABLE_ERROR:bool      = 1");
                DUMP_LINE(dump, "ENABLE_FATAL:bool      = 1");
                DUMP_LINE(dump, "ENABLE_DEBUG:bool      = 1");
                DUMP_LINE(dump, "HEADER_FORMAT:string   =[{TYPE} {DATE}] : ");
                dump.close();
            }
        }
    }
#   undef DUMP_LINE
#   undef READ_BOOL
#   undef __DETAILS_FRIENDSHIPS
#   undef MTL_LOG_FRIEND
}
#endif
