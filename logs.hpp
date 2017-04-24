#ifndef MTL_LOGS_HPP_INCLUDED
#define MTL_LOGS_HPP_INCLUDED

#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <map>

// if -pthread or -fopenmp provided only
#ifdef _REENTRANT
    // For now, when using windows there's no multithreading feature
#   if !defined _WIN32 && !defined __CYGWIN__
#       define MTL_LOG_WITH_THREADS
#   endif
#endif
#ifdef MTL_LOG_WITH_THREADS
#   include <thread>
#   include <mutex>
#   define MTL_THREAD_ID std::thread::id
#else
    // As Threads are not used here, a fake thread id type can be used safely
#   define MTL_THREAD_ID int
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

#define MTL_LOG_VARIABLE(varname) #varname " =", varname

#ifdef MTL_LOG_WITH_THREADS
#define MTL_LOG_IF_WITH_THREAD(code) code
#else
#define MTL_LOG_IF_WITH_THREAD(code)
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
    MTL_LOG_DECLARE(trace)
    inline DECLSPEC bool loadConfiguration(const std::string& fname);
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
            MTL_LOG_FRIEND(debug) \
            MTL_LOG_FRIEND(trace)
        
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
                operator std::string(void)
                {
                    return this->pattern;
                }
                void display(std::ostream& out, const std::string& type, const char *const color,
                             const char *const nocolor, bool colorEnabled
#                            ifdef MTL_LOG_WITH_THREADS
                             , const std::map<MTL_THREAD_ID, std::string>& threads_names
#                            endif // MTL_LOG_WITH_THREADS
                             )
                {
                    for(const auto& p : this->chunks)
                    {
                        switch(p.first)
                        {
                            case -1:
                            {
                                time_t  tt  = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                                std::tm ltm = *localtime(&tt);
                                this->printTwoDigits(out, ltm.tm_mon+1) << '/';
                                this->printTwoDigits(out, ltm.tm_mday)  << '/';
                                out << ltm.tm_year + 1900;
                                break;
                            }
                            case -4:
                            {
                                time_t  tt  = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                                std::tm ltm = *localtime(&tt);
                                this->printTwoDigits(out, ltm.tm_hour) << ':';
                                this->printTwoDigits(out, ltm.tm_min) << ':';
                                this->printTwoDigits(out, ltm.tm_sec);
                                break;
                            }
                            case -3:
#                           ifdef MTL_LOG_WITH_THREADS
                                try
                                {
                                    out << threads_names.at(std::this_thread::get_id());
                                }
                                catch (const std::out_of_range&)
                                {
                                    out << "0x" << std::hex << std::this_thread::get_id() << std::dec;
                                }
#                           endif // MTL_LOG_WITH_THREADS
                                break;
                            case -2:
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
                std::string                      pattern;
                std::vector<std::pair<int, int>> chunks;
                inline std::ostream& printTwoDigits(std::ostream& out, int value)
                {
                    return out << ((value < 10) ? "0" : "") << value;
                }
                void splitAndMerge(int begin, int end, int id)
                {
                    for(auto it=this->chunks.begin();it!=this->chunks.end();++it)
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
                void checkAndMergeIfFound(const std::string& tag, int id)
                {
                    std::size_t pos = this->pattern.find(tag);
                    if (pos != std::string::npos)
                    {
                        this->splitAndMerge(static_cast<int>(pos), static_cast<int>(tag.length()), id);
                    }
                }
                void build(void)
                {
                    this->chunks.clear();
                    this->chunks.push_back({0, static_cast<int>(this->pattern.size())});
                    this->checkAndMergeIfFound("{TYPE}",   -2);
                    this->checkAndMergeIfFound("{DATE}",   -1);
                    this->checkAndMergeIfFound("{THREAD}", -3);
                    this->checkAndMergeIfFound("{TIME}",   -4);
                }
        };
        template<typename T>
        class __Static_declarer
        {
            protected:
                static bool                                   ENABLE_LOG;
                static bool                                   ENABLE_COLOR;
                static bool                                   ENABLE_SPACING;
                static bool                                   ENABLE_DEBUG;
                static bool                                   ENABLE_INFO;
                static bool                                   ENABLE_ERROR;
                static bool                                   ENABLE_WARNING;
                static bool                                   ENABLE_FATAL;
                static bool                                   ENABLE_TRACE;
                static bool                                   ENABLE_HEADER;
                static std::ostream*                          OUT;
                static bool                                   ENABLE_ALPHA_BOOL;
                static MTL_LOG_NAMESPACE::__details::__Header FORMAT;
                
#               ifdef MTL_LOG_WITH_THREADS
                static std::map<MTL_THREAD_ID, std::string> THREAD_NAME;
#               endif
            private:
#               ifdef MTL_LOG_WITH_THREADS
                static std::mutex MUTEX;
#               endif
                static constexpr const char *const C_YELLOW = "\033[1;33m";
                static constexpr const char *const C_RED    = "\033[1;31m";
                static constexpr const char *const C_GREEN  = "\033[1;32m";
                static constexpr const char *const C_BLUE   = "\033[1;36m";
                static constexpr const char *const C_BLANK  = "\033[0m";
                friend class MTL_LOG_NAMESPACE::__details::_Logger;
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
        STATIC_DECLARATION(bool,          ENABLE_TRACE,      true)
        STATIC_DECLARATION(bool,          ENABLE_HEADER,     true)
        STATIC_DECLARATION(std::ostream*, OUT,               &std::cout)
        STATIC_DECLARATION(bool,          ENABLE_ALPHA_BOOL, true)
#       ifdef MTL_LOG_WITH_THREADS
        template<typename T> std::mutex __Static_declarer<T>::MUTEX;
        template<typename T> std::map<MTL_THREAD_ID, std::string> __Static_declarer<T>::THREAD_NAME = {};
#       endif
        STATIC_DECLARATION(MTL_LOG_NAMESPACE::__details::__Header, FORMAT, std::string("[{TYPE} {DATE} {TIME}] : "))
#       undef STATIC_DECLARATION
    }
#   ifndef MTL_LOG_WITH_THREAD
#       define MTL_LOG_LOCK \
            do{}while(false)
#   else
#       define MTL_LOG_LOCK \
            std::unique_lock<std::mutex> lck(MTL_LOG_NAMESPACE::Options::MUTEX)
#   endif
#   define MTL_LOG_GET_SET(type, name, option) \
        static void enable##name(type value) noexcept\
        {\
            MTL_LOG_LOCK;\
            MTL_LOG_NAMESPACE::Options::option = value;\
        }\
        static type is##name##Enabled(void) noexcept\
        {\
            MTL_LOG_LOCK;\
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
            MTL_LOG_GET_SET(bool, Header,    ENABLE_HEADER)
            MTL_LOG_GET_SET(bool, Trace,     ENABLE_TRACE)
            static void setOutputStream(std::ostream* out)
            {
                MTL_LOG_LOCK;
                MTL_LOG_NAMESPACE::Options::OUT = out;
            }
            static std::ostream* getOutputStream(void)
            {
                MTL_LOG_LOCK;
                return MTL_LOG_NAMESPACE::Options::OUT;
            }
            static void setFormat(const std::string& format)
            {
                MTL_LOG_LOCK;
                MTL_LOG_NAMESPACE::Options::FORMAT = format;
            }
            static std::string getFormat(void)
            {
                MTL_LOG_LOCK;
                return MTL_LOG_NAMESPACE::Options::FORMAT;
            }
            static void bindThreadName(const std::string& MTL_LOG_IF_WITH_THREAD(name))
            {
#           ifdef MTL_LOG_WITH_THREADS
                MTL_LOG_LOCK;
                MTL_LOG_NAMESPACE::Options::THREAD_NAME.insert(std::make_pair(std::this_thread::get_id(), name));
#           endif
            }
            static void bindThreadName(const MTL_THREAD_ID& MTL_LOG_IF_WITH_THREAD(id), const std::string& MTL_LOG_IF_WITH_THREAD(name))
            {
#           ifdef MTL_LOG_WITH_THREADS
                MTL_LOG_LOCK;
                MTL_LOG_NAMESPACE::Options::THREAD_NAME.insert(std::make_pair(id, name));
#           endif
            }
            static void unbindThreadName(const MTL_THREAD_ID& MTL_LOG_IF_WITH_THREAD(id = std::this_thread::get_id()))
            {
#           ifdef MTL_LOG_WITH_THREADS
                MTL_LOG_LOCK;
                MTL_LOG_NAMESPACE::Options::THREAD_NAME.erase(id);
#           endif
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
                template<typename Actual>
                static void _print_(const Actual& a)
                {
                    *MTL_LOG_NAMESPACE::Options::OUT << a << std::endl;
                }
                template<typename Actual, typename... Args>
                static void _print_(const Actual& a, const Args&... args)
                {
                    *MTL_LOG_NAMESPACE::Options::OUT << a;
                    if (MTL_LOG_NAMESPACE::Options::ENABLE_SPACING)
                    {
                        *MTL_LOG_NAMESPACE::Options::OUT << ' ';
                    }
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
                    MTL_LOG_LOCK;
                    MTL_LOG_NAMESPACE::__details::_Logger::assertValidity();
                    *MTL_LOG_NAMESPACE::Options::OUT << ((MTL_LOG_NAMESPACE::Options::isAlphaBoolEnabled()) ? std::boolalpha
                                                                                                            : std::noboolalpha);
                    if (MTL_LOG_NAMESPACE::Options::isHeaderEnabled())
                    {
                        MTL_LOG_NAMESPACE::Options::FORMAT.display(*MTL_LOG_NAMESPACE::Options::OUT,
                                                                   tag, color,
                                                                   MTL_LOG_NAMESPACE::Options::C_BLANK,
                                                                   MTL_LOG_NAMESPACE::Options::isColorEnabled()
#                                                                  ifdef MTL_LOG_WITH_THREADS
                                                                   , MTL_LOG_NAMESPACE::Options::THREAD_NAME
#                                                                  endif
                                                                   );
                    }
                    MTL_LOG_NAMESPACE::__details::_Logger::_print_(args...);
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
    MTL_LOG_METHOD(trace,   "TRACE  ", C_BLUE,   Trace)
#   undef MTL_LOG_METHOD
#   undef MTL_LOG_LOCK
    
#   define MTL_LOG_DUMP_LINE(file, text) file << text << std::endl;
#   define MTL_LOG_READ_BOOL(varname) \
        config >> foo >> equal >> value;\
        MTL_LOG_NAMESPACE::Options::enable##varname(value);
    inline DECLSPEC bool loadConfiguration(const std::string& fname)
    {
        std::ifstream config(fname);
        if (config.good())
        {
            std::string foo;
            char        equal;
            bool        value;
            MTL_LOG_READ_BOOL(Log);
            MTL_LOG_READ_BOOL(Color);
            MTL_LOG_READ_BOOL(Spacing);
            MTL_LOG_READ_BOOL(AlphaBool);
            MTL_LOG_READ_BOOL(Info);
            MTL_LOG_READ_BOOL(Warning);
            MTL_LOG_READ_BOOL(Error);
            MTL_LOG_READ_BOOL(Fatal);
            MTL_LOG_READ_BOOL(Debug);
            MTL_LOG_READ_BOOL(Trace);
            MTL_LOG_READ_BOOL(Header);
            config >> foo >> equal;
            std::getline(config, foo);
            MTL_LOG_NAMESPACE::Options::setFormat(foo);
            config.close();
            return true;
        }
        else
        {
            std::ofstream dump(fname);
            if (dump.good())
            {
                MTL_LOG_DUMP_LINE(dump, "ENABLE_LOG:bool        = 1");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_COLOR:bool      = 0");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_SPACING:bool    = 1");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_ALPHA_BOOL:bool = 1");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_INFO:bool       = 1");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_WARNING:bool    = 1");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_ERROR:bool      = 1");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_FATAL:bool      = 1");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_DEBUG:bool      = 1");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_TRACE:bool      = 1");
                MTL_LOG_DUMP_LINE(dump, "ENABLE_HEADER:bool     = 1");
                MTL_LOG_DUMP_LINE(dump, "HEADER_FORMAT:string   =[{TYPE} {DATE}] : ");
                dump.close();
            }
            return false;
        }
    }
#   undef MTL_LOG_DUMP_LINE
#   undef MTL_LOG_READ_BOOL
#   undef __DETAILS_FRIENDSHIPS
#   undef MTL_LOG_FRIEND
#   undef MTL_LOG_WITH_THREADS
}
#endif
