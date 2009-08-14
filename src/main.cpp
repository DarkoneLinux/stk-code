// $Id$
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2005 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2006 Joerg Henrichs, Steve Baker
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#ifdef WIN32
#  ifdef __CYGWIN__
#    include <unistd.h>
#  endif
#  define _WINSOCKAPI_
#  include <windows.h>
#  ifdef _MSC_VER
#    include <io.h>
#    include <direct.h>
#  endif
#else
#  include <unistd.h>
#endif
#include <stdexcept>
#include <cstdio>
#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>

#include "main_loop.hpp"
#include "audio/sound_manager.hpp"
#include "audio/sfx_manager.hpp"
#include "challenges/unlock_manager.hpp"
#include "config/stk_config.hpp"
#include "config/user_config.hpp"
#include "config/player.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/scene.hpp"
#include "guiengine/engine.hpp"
#include "states_screens/state_manager.hpp"
#include "io/file_manager.hpp"
#include "input/input_manager.hpp"
#include "input/device_manager.hpp"
#include "items/attachment_manager.hpp"
#include "items/item_manager.hpp"
#include "items/projectile_manager.hpp"
#include "karts/kart_properties_manager.hpp"
#include "karts/kart.hpp"
#include "network/network_manager.hpp"
#include "race/grand_prix_manager.hpp"
#include "race/highscore_manager.hpp"
#include "race/history.hpp"
#include "race/race_manager.hpp"
#include "tracks/track.hpp"
#include "tracks/track_manager.hpp"
#include "utils/translation.hpp"

// Only needed for bullet debug!
#ifdef HAVE_GLUT
#  ifdef __APPLE__
#    include <GLUT/glut.h>
#  else
#    include <GL/glut.h>
#  endif
#endif

void cmdLineHelp (char* invocation)
{
    fprintf ( stdout,
    "Usage: %s [OPTIONS]\n\n"
    "Run SuperTuxKart, a racing game with go-kart that features"
    " the Tux and friends.\n\n"
    "Options:\n"
    "  -N,  --no-start-screen  Quick race\n"
    "  -t,  --track NAME       Start at track NAME (see --list-tracks)\n"
    "       --stk-config FILE  use ./data/FILE instead of ./data/stk_config.data\n"
    "  -l,  --list-tracks      Show available tracks\n"
    "  -k,  --numkarts NUM     Number of karts on the racetrack\n"
    "       --kart NAME        Use kart number NAME (see --list-karts)\n"
    "       --list-karts       Show available karts\n"
    "       --laps N           Define number of laps to N\n"
    "       --mode N           N=1 novice, N=2 driver, N=3 racer\n"
    //FIXME"     --players n             Define number of players to between 1 and 4.\n"
    //FIXME     "  --reverse               Enable reverse mode\n"
    //FIXME     "  --mirror                Enable mirror mode (when supported)\n"
    "       --item STYLE       Use STYLE as your item style\n"
    "  -f,  --fullscreen       Fullscreen display\n"
    "  -w,  --windowed         Windowed display (default)\n"
    "  -s,  --screensize WxH   Set the screen size (e.g. 320x200)\n"
    "  -v,  --version          Show version\n"
    // should not be used by unaware users:
    // "  --profile            Enable automatic driven profile mode for 20 seconds\n"
    // "  --profile=n          Enable automatic driven profile mode for n seconds\n"
    // "                       if n<0 --> (-n) = number of laps to drive
    // "  --history            Replay history file 'history.dat'\n"
    // "  --history=n          Replay history file 'history.dat' using mode:\n"
    // "                       n=1: use recorded positions\n"
    // "                       n=2: use recorded key strokes\n"
    "  --server[=port]         This is the server (running on the specified port)\n"
    "  --client=ip             This is a client, connect to the specified ip address\n"
    "  --port=n                Port number to use\n"
    "  --numclients=n          Number of clients to wait for (server only)\n"
    "  --log=terminal          Write messages to screen\n"
    "  --log=file              Write messages/warning to log files stdout.log/stderr.log\n"
    "  -h,  --help             Show this help\n"
    "\n"
    "You can visit SuperTuxKart's homepage at "
    "http://supertuxkart.sourceforge.net\n\n", invocation
    );
}   // cmdLineHelp

//=============================================================================
// for base options that don't need much to be inited (and, in some cases, that need to
// be read before initing stuff)
int handleCmdLinePreliminary(int argc, char **argv)
{
    for(int i=1; i<argc; i++)
    {
        if(argv[i][0] != '-') continue;
        if ( !strcmp(argv[i], "--help"    ) ||
            !strcmp(argv[i], "-help"     ) ||
            !strcmp(argv[i], "--help" ) ||
            !strcmp(argv[i], "-help" ) ||
            !strcmp(argv[i], "-h")       )
        {
            cmdLineHelp(argv[0]);
            exit(0);
        }
#if !defined(WIN32) && !defined(__CYGWIN)
        else if ( !strcmp(argv[i], "--fullscreen") || !strcmp(argv[i], "-f"))
        {
            // Check that current res is not blacklisted
            std::ostringstream o;
    		o << UserConfigParams::m_width << "x" << UserConfigParams::m_height;
    		std::string res = o.str();
            if (std::find(UserConfigParams::m_blacklist_res.begin(), 
                          UserConfigParams::m_blacklist_res.end(),res) == UserConfigParams::m_blacklist_res.end())         
            	UserConfigParams::m_fullscreen = true;
          	else 
          		fprintf ( stdout, "Resolution %s has been blacklisted, so it is not available!\n", res.c_str());
        }
        else if ( !strcmp(argv[i], "--windowed") || !strcmp(argv[i], "-w"))
        {
            UserConfigParams::m_fullscreen = false;
        }
#endif
        else if ( !strcmp(argv[i], "--screensize") || !strcmp(argv[i], "-s") )
        {
            //Check if fullscreen and new res is blacklisted
            int width, height;
            if (sscanf(argv[i+1], "%dx%d", &width, &height) == 2)
            {
            	std::ostringstream o;
    			o << width << "x" << height;
    			std::string res = o.str();
                if (!UserConfigParams::m_fullscreen || std::find(UserConfigParams::m_blacklist_res.begin(), 
                                                            UserConfigParams::m_blacklist_res.end(),res) == UserConfigParams::m_blacklist_res.end())
                {
                	UserConfigParams::m_prev_width = UserConfigParams::m_width = width;
               		UserConfigParams::m_prev_height = UserConfigParams::m_height = height;
                	fprintf ( stdout, "You choose to be in %dx%d.\n", (int)UserConfigParams::m_width,
                             (int)UserConfigParams::m_height );
               	}
               	else
               		fprintf ( stdout, "Resolution %s has been blacklisted, so it is not available!\n", res.c_str());
            }
            else
            {
                fprintf(stderr, "Error: --screensize argument must be given as WIDTHxHEIGHT\n");
                exit(EXIT_FAILURE);
            }
        }
        else if( !strcmp(argv[i], "--version") ||  !strcmp(argv[i], "-v") )
        {
            printf("==============================\n");
#ifdef VERSION
            fprintf ( stdout, "SuperTuxKart, %s.\n", VERSION ) ;
#endif
#ifdef SVNVERSION
            fprintf ( stdout, "SuperTuxKart, SVN revision number '%s'.\n", SVNVERSION ) ;
#endif
#if !defined(VERSION) && !defined(SVNVERSION)
            fprintf ( stdout, "SuperTuxKart, unknown version\n" ) ;
#endif
            printf("==============================\n");
            exit(0);
        }
    }
    return 0;
}

int handleCmdLine(int argc, char **argv)
{
    int n;
    char s[80];
    for(int i=1; i<argc; i++)
    {
        if(argv[i][0] != '-') continue;
        if(!strcmp(argv[i], "--gamepad-debug"))
        {
            UserConfigParams::m_gamepad_debug=true;
        }
        else if(sscanf(argv[i], "--track-debug=%d",&n)==1)
        {
            UserConfigParams::m_track_debug=n;
        }
        else if(!strcmp(argv[i], "--track-debug"))
        {
            UserConfigParams::m_track_debug=1;
        }
#ifdef HAVE_GLUT
        else if(!strcmp(argv[i], "--bullet-debug"))
        {
            UserConfigParams::m_bullet_debug=1;
        }
#endif
        else if(!strcmp(argv[i], "--kartsize-debug"))
        {
            UserConfigParams::m_print_kart_sizes=true;
        }
        else if(sscanf(argv[i], "--server=%d",&n)==1)
        {
            network_manager->setMode(NetworkManager::NW_SERVER);
            UserConfigParams::m_server_port = n;
        }
        else if( !strcmp(argv[i], "--server") )
        {
            network_manager->setMode(NetworkManager::NW_SERVER);
        }
        else if( sscanf(argv[i], "--port=%d", &n) )
        {
            UserConfigParams::m_server_port=n;
        }
        else if( sscanf(argv[i], "--client=%s", s) )
        {
            network_manager->setMode(NetworkManager::NW_CLIENT);
            UserConfigParams::m_server_address=s;
        }
        else if( (!strcmp(argv[i], "--kart") && i+1<argc ))
        {
            std::string filename=file_manager->getKartFile(std::string(argv[i+1])+".tkkf");
            if(filename!="")
            {
                UserConfigParams::m_default_kart = argv[i+1];
                
                // if a player was added with -N, change its kart. Otherwise, nothing to do,
                // kart choice will be picked up upon player creation.
                if (StateManager::get()->activePlayerCount() > 0)
                {
                    race_manager->setNumLocalPlayers(1);
                    race_manager->setLocalKartInfo(0, argv[i+1]);
                }
                fprintf ( stdout, "You chose to use kart '%s'.\n", argv[i+1] ) ;
            }
            else
            {
	            fprintf(stdout, "Kart '%s' not found, ignored.\n",
		                argv[i+1]);
            }
        }
        else if( (!strcmp(argv[i], "--mode") && i+1<argc ))
        {
            switch (atoi(argv[i+1]))
            {
            case 1:
                race_manager->setDifficulty(RaceManager::RD_EASY);
                break;
            case 2:
                race_manager->setDifficulty(RaceManager::RD_MEDIUM);
                break;
            case 3:
                race_manager->setDifficulty(RaceManager::RD_HARD);
                break;
            }
        }
        else if( (!strcmp(argv[i], "--track") || !strcmp(argv[i], "-t"))
                 && i+1<argc                                              )
        {
            if (!unlock_manager->isLocked(argv[i+1]))
            {
                race_manager->setTrack(argv[i+1]);
                fprintf ( stdout, "You choose to start in track: %s.\n", argv[i+1] ) ;
            }
            else 
            {
                fprintf(stdout, "Track %s has not been unlocked yet. \n", argv[i+1]);
                fprintf(stdout, "Use --list-tracks to list available tracks.\n\n");
                return 0;
            }
            i++;
        }
        else if( (!strcmp(argv[i], "--stk-config")) && i+1<argc )
        {
            stk_config->load(file_manager->getConfigFile(argv[i+1]));
            fprintf ( stdout, "STK config will be read from %s.\n", argv[i+1] ) ;
        }
        else if( (!strcmp(argv[i], "--numkarts") || !strcmp(argv[i], "-k")) &&
                 i+1<argc )
        {
            UserConfigParams::m_num_karts = atoi(argv[i+1]);
            if(UserConfigParams::m_num_karts > stk_config->m_max_karts)
            {
                fprintf(stdout, "Number of karts reset to maximum number %d\n",
                                  stk_config->m_max_karts);
                UserConfigParams::m_num_karts = stk_config->m_max_karts;
            }
            race_manager->setNumKarts( UserConfigParams::m_num_karts );
            fprintf(stdout, "%d karts will be used.\n",  (int)UserConfigParams::m_num_karts);
            i++;
        }
        else if( !strcmp(argv[i], "--list-tracks") || !strcmp(argv[i], "-l") )
        {

            fprintf ( stdout, "  Available tracks:\n" );
            for (size_t i = 0; i != track_manager->getNumberOfTracks(); i++)
            {
                const Track *track = track_manager->getTrack(i);
                if (!unlock_manager->isLocked(track->getIdent()))
                {
                    fprintf ( stdout, "\t%10s: %s\n",
                              track->getIdent().c_str(),
                              track->getName().c_str());
                }
            }    

            fprintf ( stdout, "Use --track N to choose track.\n\n");
            delete track_manager;
            track_manager = 0;

            return 0;
        }
        else if( !strcmp(argv[i], "--list-karts") )
        {
            bool dont_load_models=true;
            kart_properties_manager->loadKartData(dont_load_models) ;

            fprintf ( stdout, "  Available karts:\n" );
            for (unsigned int i = 0; NULL != kart_properties_manager->getKartById(i); i++)
            {
                const KartProperties* KP= kart_properties_manager->getKartById(i);
                fprintf (stdout, "\t%10s: %s\n", KP->getIdent().c_str(), KP->getName().c_str());
            }
            fprintf ( stdout, "\n" );

            return 0;
        }
        else if (    !strcmp(argv[i], "--no-start-screen")
                     || !strcmp(argv[i], "-N")                )
        {
            UserConfigParams::m_no_start_screen = true;
            //FIXME} else if ( !strcmp(argv[i], "--reverse") ) {
            //FIXME:fprintf ( stdout, "Enabling reverse mode.\n" ) ;
            //FIXME:raceSetup.reverse = 1;
        }
        else if ( !strcmp(argv[i], "--mirror") )
        {
#ifdef SSG_BACKFACE_COLLISIONS_SUPPORTED
            fprintf ( stdout, "Enabling mirror mode.\n" ) ;
            //raceSetup.mirror = 1;
#else
            //raceSetup.mirror = 0 ;
#endif

        }
        else if ( !strcmp(argv[i], "--laps") && i+1<argc )
        {
            fprintf ( stdout, "You choose to have %d laps.\n", atoi(argv[i+1]) ) ;
            race_manager->setNumLaps(atoi(argv[i+1]));
        }
        /* FIXME:
        else if ( !strcmp(argv[i], "--players") && i+1<argc ) {
          raceSetup.numPlayers = atoi(argv[i+1]);

          if ( raceSetup.numPlayers < 0 || raceSetup.numPlayers > 4) {
        fprintf ( stderr,
        "You choose an invalid number of players: %d.\n",
        raceSetup.numPlayers );
        cmdLineHelp(argv[0]);
        return 0;
          }
          fprintf ( stdout, "You choose to have %d players.\n", atoi(argv[i+1]) ) ;
        }
        */
        else if( !strcmp(argv[i], "--log=terminal"))
        {
            UserConfigParams::m_log_errors=false;
        }
        else if( !strcmp(argv[i], "--log=file"))
        {
            UserConfigParams::m_log_errors=true;
        } else if( sscanf(argv[i], "--profile=%d",  &n)==1)
        {
            UserConfigParams::m_profile=n;
	    if(n<0) 
	    {
                fprintf(stdout,"Profiling %d laps\n",-n);
                race_manager->setNumLaps(-n);
	    }
	    else
            {
	        printf("Profiling: %d seconds.\n", (int)UserConfigParams::m_profile);
                race_manager->setNumLaps(999999); // profile end depends on time
            }
        }
        else if( !strcmp(argv[i], "--profile") )
        {
            UserConfigParams::m_profile=20;
        }
        else if( sscanf(argv[i], "--history=%d",  &n)==1)
        {
            history->doReplayHistory( (History::HistoryReplayMode)n);
        }
        else if( !strcmp(argv[i], "--history") )
        {
            history->doReplayHistory(History::HISTORY_POSITION);
        }
        else if( !strcmp(argv[i], "--item") && i+1<argc )
        {
            item_manager->setUserFilename(argv[i+1]);
        }
        // these commands are already processed in handleCmdLinePreliminary, but repeat this
        // just so that we don't get error messages about unknown commands
        else if ( !strcmp(argv[i], "--fullscreen") || !strcmp(argv[i], "-f"))
        {
        }
        else if ( !strcmp(argv[i], "--windowed") || !strcmp(argv[i], "-w"))
        {
        }
        else if ( !strcmp(argv[i], "--screensize") || !strcmp(argv[i], "-s") )
        {
        }
        else if ( !strcmp(argv[i], "--version") || !strcmp(argv[i], "-v") )
        {
        }
        else
        {
            fprintf ( stderr, "Invalid parameter: %s.\n\n", argv[i] );
            cmdLineHelp(argv[0]);
            return 0;
        }
    }   // for i <argc
    if(UserConfigParams::m_profile)
    {
        UserConfigParams::m_sfx = false;  // Disable sound effects 
        UserConfigParams::m_music = false;// and music when profiling
    }

    return 1;
}   /* handleCmdLine */

//=============================================================================
void initPreliminary()
{
    file_manager            = new FileManager();
    translations            = new Translations();
    // unlock manager is needed when reading the config file
    unlock_manager          = new UnlockManager();
    user_config             = new UserConfig();
}
void initRest()
{
    irr_driver              = new IrrDriver();
    sound_manager           = new SoundManager();
    sfx_manager             = new SFXManager();
    // The order here can be important, e.g. KartPropertiesManager needs
    // defaultKartProperties.
    history                 = new History              ();
    material_manager        = new MaterialManager      ();
    track_manager           = new TrackManager         ();
    stk_config              = new STKConfig            ();
    kart_properties_manager = new KartPropertiesManager();
    projectile_manager      = new ProjectileManager    ();
    powerup_manager         = new PowerupManager       ();
    item_manager            = new ItemManager          ();
    attachment_manager      = new AttachmentManager    ();
    highscore_manager       = new HighscoreManager     ();
    grand_prix_manager      = new GrandPrixManager     ();
    network_manager         = new NetworkManager       ();

    stk_config->load(file_manager->getConfigFile("stk_config.data"));
    track_manager->loadTrackList();
    // unlock_manager->check needs GP and track manager.
    unlock_manager->check();
    sound_manager->addMusicToTracks();

    race_manager            = new RaceManager          ();
    // default settings for Quickstart
    race_manager->setNumPlayers(1);
    race_manager->setNumLaps   (3);
    race_manager->setMajorMode (RaceManager::MAJOR_MODE_SINGLE);
    race_manager->setMinorMode (RaceManager::MINOR_MODE_QUICK_RACE);
    race_manager->setDifficulty(RaceManager::RD_HARD);

    //menu_manager= new MenuManager();

    // Consistency check for challenges, and enable all challenges
    // that have all prerequisites fulfilled
    grand_prix_manager->checkConsistency();
}

//=============================================================================
void cleanTuxKart()
{
    //delete in reverse order of what they were created in.
    //see InitTuxkart()
    if(race_manager)            delete race_manager;
    if(network_manager)         delete network_manager;
    if(grand_prix_manager)      delete grand_prix_manager;
    if(highscore_manager)       delete highscore_manager;
    if(attachment_manager)      delete attachment_manager;
    if(item_manager)            delete item_manager;
    if(powerup_manager)         delete powerup_manager;   
    if(projectile_manager)      delete projectile_manager;
    if(kart_properties_manager) delete kart_properties_manager;
    if(stk_config)              delete stk_config;
    if(track_manager)           delete track_manager;
    if(material_manager)        delete material_manager;
    if(history)                 delete history;
    if(sfx_manager)             delete sfx_manager;
    if(sound_manager)           delete sound_manager;
    if(user_config)             delete user_config;
    if(unlock_manager)          delete unlock_manager;
    if(translations)            delete translations;
    if(file_manager)            delete file_manager;
    if(stk_scene)               delete stk_scene;
    if(irr_driver)              delete irr_driver;
}

//=============================================================================


int main(int argc, char *argv[] ) 
{
    try {
#ifdef HAVE_GLUT
        // only needed for bullet debugging.
        glutInit(&argc, argv);
#endif
        
        initPreliminary();
        handleCmdLinePreliminary(argc, argv);
        
        initRest();

        //handleCmdLine() needs InitTuxkart() so it can't be called first
        if(!handleCmdLine(argc, argv)) exit(0);
        
        if (UserConfigParams::m_log_errors) //Enable logging of stdout and stderr to logfile
        {
            std::string logoutfile = file_manager->getLogFile("stdout.log");
            std::string logerrfile = file_manager->getLogFile("stderr.log");
            std::cout << "Error messages and other text output will be logged to " ;
            std::cout << logoutfile << " and "<<logerrfile;
            if(freopen (logoutfile.c_str(),"w",stdout)!=stdout)
            {
                fprintf(stderr, "Can not open log file '%s'. Writing to stdout instead.\n",
                        logoutfile.c_str());
            }
            if(freopen (logerrfile.c_str(),"w",stderr)!=stderr)
            {
                fprintf(stderr, "Can not open log file '%s'. Writing to stderr instead.\n",
                        logerrfile.c_str());
            }
        }
        
        input_manager = new InputManager ();
        
        // Get into menu mode initially.
        input_manager->setMode(InputManager::MENU);  
        
        main_loop = new MainLoop();
        // loadMaterials needs ssgLoadTextures (internally), which can
        // only be called after ssgInit (since this adds the actual file_manager)
        // so this next call can't be in InitTuxkart. And InitPlib needs
        // config, which gets defined in InitTuxkart, so swapping those two
        // calls is not possible either ... so loadMaterial has to be done here :(
        material_manager        -> loadMaterial    ();
        kart_properties_manager -> loadKartData    ();
        projectile_manager      -> loadData        ();
        powerup_manager         -> loadPowerups    ();
        item_manager            -> loadDefaultItems();
        attachment_manager      -> loadModels      ();
        stk_scene = new Scene();

        // Init GUI prepare main menu
        IrrlichtDevice* device = irr_driver->getDevice();
        video::IVideoDriver* driver = device->getVideoDriver();
        GUIEngine::init(device, driver, StateManager::get());
        
        if(!UserConfigParams::m_no_start_screen)
        {
            StateManager::get()->pushMenu("main.stkgui");
        }
        else 
        {
            InputDevice *device;
            ActivePlayer* newPlayer;

            // Use keyboard by default in --no-start-screen
            device = input_manager->getDeviceList()->getKeyboard(0);

            // Create player and associate player with keyboard
            newPlayer = new ActivePlayer( UserConfigParams::m_all_players.get(0) );
            StateManager::get()->addActivePlayer(newPlayer);
            newPlayer->setDevice(device);

            // Set up race manager appropriately
            race_manager->setNumLocalPlayers(1);
            race_manager->setLocalKartInfo(0, UserConfigParams::m_default_kart);

            // ASSIGN should make sure that only input from assigned devices
            // is read.
            input_manager->getDeviceList()->setAssignMode(ASSIGN);

            // Go straight to the race
            StateManager::get()->enterGameState();
        }

        // Replay a race
        // =============
        if(history->replayHistory())
        {
            // This will setup the race manager etc.
            history->Load();
            network_manager->setupPlayerKartInfo();
            race_manager->startNew();
            main_loop->run();
            // well, actually run() will never return, since
            // it exits after replaying history (see history::GetNextDT()).
            // So the next line is just to make this obvious here!
            exit(-3);
        }

        // Initialise connection in case that a command line option was set
        // configuring a client or server. Otherwise this function does nothing
        // here (and will be called again from the network gui).
        if(!network_manager->initialiseConnections())
        {
            fprintf(stderr, "Problems initialising network connections,\n"
                            "Running in non-network mode.\n");
        }
        // On the server start with the network information page for now
        if(network_manager->getMode()==NetworkManager::NW_SERVER)
        {
            // TODO - network menu
            //menu_manager->pushMenu(MENUID_NETWORK_GUI);
        }
        // Not replaying
        // =============
        if(!UserConfigParams::m_profile)
        {
            if(UserConfigParams::m_no_start_screen)
            {
                // Quickstart (-N)
                // ===============
                // all defaults are set in InitTuxkart()
                network_manager->setupPlayerKartInfo();
                race_manager->startNew();
            }
        }
        else  // profile
        {
            // Profiling
            // =========
            race_manager->setMajorMode (RaceManager::MAJOR_MODE_SINGLE);
            race_manager->setMinorMode (RaceManager::MINOR_MODE_QUICK_RACE);
            race_manager->setDifficulty(RaceManager::RD_HARD);
            network_manager->setupPlayerKartInfo();
            race_manager->startNew();
        }
        main_loop->run();

    }  // try
    catch (std::exception &e)
    {
        fprintf(stderr,"%s",e.what());
        fprintf(stderr,"\nAborting SuperTuxKart\n");
    }

    /* Program closing...*/

    if(user_config)
    {
        // In case that abort is triggered before user_config exists
        if (UserConfigParams::m_crashed) UserConfigParams::m_crashed = false;
        user_config->saveConfig();
    }
    if(input_manager) delete input_manager;  // if early crash avoid delete NULL
    
    if (user_config && UserConfigParams::m_log_errors) //close logfiles
    {
        fclose(stderr);
        fclose(stdout);
    }

    cleanTuxKart();

    return 0 ;
}

