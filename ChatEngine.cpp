/**
  * RealBot : Artificial Intelligence
  * Version : Work In Progress
  * Author  : Stefan Hendriks
  * Url     : http://realbot.bots-united.com
  **
  * DISCLAIMER
  *
  * History, Information & Credits: 
  * RealBot is based partially upon the HPB-Bot Template #3 by Botman
  * Thanks to Ditlew (NNBot), Pierre Marie Baty (RACCBOT), Tub (RB AI PR1/2/3)
  * Greg Slocum & Shivan (RB V1.0), Botman (HPB-Bot) and Aspirin (JOEBOT). And
  * everybody else who helped me with this project.
  * Storage of Visibility Table using BITS by Cheesemonster.
  *
  * Some portions of code are from other bots, special thanks (and credits) go
  * to (in no specific order):
  *
  * Pierre Marie Baty
  * Count-Floyd
  *  
  * !! BOTS-UNITED FOREVER !!
  *  
  * This project is open-source, it is protected under the GPL license;
  * By using this source-code you agree that you will ALWAYS release the
  * source-code with your project.
  *
  **/

// Chatting Engine
#include <cstring>
#include <cctype>
// Some tests by EVYNCKE
#include <string>
#include <algorithm>

#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "IniParser.h"
#include "bot_weapons.h"
#include "game.h"
#include "bot_func.h"

#include "ChatEngine.h"

extern edict_t* pHostEdict;
extern cGame Game;
extern cBot bots[32];

// initialize all
void cChatEngine::init() {
    // clear all blocks
    for (tReplyBlock& iB : ReplyBlock)
    {
        for (char(&iBs)[128] : iB.sentence)
            iBs[0] = '\0';

        for (char(&iBw)[25] : iB.word)
            iBw[0] = '\0';

        iB.bUsed = false;
    }

    iLastBlock = -1;
    iLastSentence = -1;

    // init sentence
    std::memset(sentence, 0, sizeof(sentence));
    std::memset(sender, 0, sizeof(sender));
}

// load
void cChatEngine::initAndload() {
    init();

    // load blocks using INI parser
    INI_PARSE_CHATFILE();
}

// think
void cChatEngine::think() {
    if (fThinkTimer + 1.0f > gpGlobals->time) return; // not time yet to think

    // decrease over time to avoid flooding
    if (Game.iProducedSentences > 1) {
        Game.iProducedSentences--;
    }

    // if no sender is set, do nothing
    if (sender[0] == '\0') return;

    // 29/08/2019 Stefan: by using string compare on the name of the sender (ie sender[] is the name) we retrieve
    // the edict pointer
    edict_t* pSender = nullptr;
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t* pPlayer = INDEXENT(i);

        if (pPlayer && !pPlayer->free) {
            char name[30] = {}, name2[30] = {};

            // copy
            std::strcpy(name, STRING(pPlayer->v.netname));
            std::strcpy(name2, sender);

            if (std::strcmp(name, name2) == 0) {
                pSender = pPlayer;
                break;
            }
        }
    }
    // Edict pointer established

    // Scan the message so we know in what block we should be to reply:
    char word[20] = {};

    int c = 0;
    int wc = 0;

    const int sentenceLength = static_cast<int>(std::strlen(sentence));

    // When length is not valid, get out.
    // 29/08/2019: Stefan, so let me get this. We declare the sentence to be max 128 chars, but then we still could end up with a longer one?
    // how did we allow for this to happen?
    if (sentenceLength == 0 || sentenceLength >= MAX_SENTENCE_LENGTH - 1) {
        // clear out sentence and sender
        std::memset(sentence, 0, sizeof(sentence));
        std::memset(sender, 0, sizeof(sender));

        // reset timer
        fThinkTimer = gpGlobals->time;

        return;
    }

    int WordBlockScore[MAX_BLOCKS];

    // Init, none of the block has a score yet (set to -1)
    for (int& wbs : WordBlockScore)
    {
        wbs = -1;
    }

    // loop over the sentence character by character
    while (c < sentenceLength) {
        // protection matters:
        // Stefan: this is weird, this can never happen?!
        if (c > sentenceLength)
            break;

        if (c < 0)
            break;
        // End of protection matters

        // Step: Check character to identify the end of a word.
        if (sentence[c] == ' ' || sentence[c] == '\n' ||
            sentence[c] == '.' || sentence[c] == '?' ||
            sentence[c] == '!' || c == sentenceLength) {
            // Now find the word and add up scores on the proper score blocks.

            if (c == sentenceLength)
                word[wc] = sentence[c];

            // not a good word (too small)
            if (std::strlen(word) <= 0) {
                //SERVER_PRINT("This is not a good word!\n");
            }
            else {
                for (int iB = 0; iB < MAX_BLOCKS; iB++) {
                    if (ReplyBlock[iB].bUsed) {
                        for (const char(&iBw)[25] : ReplyBlock[iB].word)
                        {
                            // skip any word in the reply block that is not valid
                            if (iBw[0] == '\0')
                                continue; // not filled in

                            if (std::strlen(iBw) <= 0)
                                continue; // not long enough (a space?)

                            // 03/07/04
                            // add score to matching word (evy: ignoring case)
                            if (std::strcmp(iBw, word) == 0)
                                WordBlockScore[iB]++;
                        }       // all words in this block
                    }          // any used block
                }             // for all blocks
            }                // good word

            // clear out entire word.
            //for (int cw=0; cw < 20; cw++)
            //      word[cw] = '\0';
            std::memset(word, 0, sizeof(word));

            wc = 0;          // reset WC position (start writing 'word[WC]' at 0 again)
            c++;             // next position in sentence
            continue;        // go to top again.
        }
        // when we end up here, we are still reading a 'non finishing word' character.
        // we will fill that in word[wc]. Then add up wc and c, until we find a character
        // that marks the end of a word again.

        // fill in the word:
        word[wc] = sentence[c];

        // add up.
        c++;
        wc++;
    }                      // end of loop

    // now loop through all blocks and find the one with the most score:
    int iMaxScore = -1;
    int iTheBlock = -1;

    // for all blocks
    for (int rB = 0; rB < MAX_BLOCKS; rB++) {
        // Any block that has the highest score
        if (WordBlockScore[rB] > iMaxScore) {
            iMaxScore = WordBlockScore[rB];
            iTheBlock = rB;
        }
    }

    // When we have found pSender edict AND we have a block to reply from
    // we continue here.
    if (pSender && iTheBlock > -1) {
        int iMax = -1;

        // now choose a sentence to reply with
        for (const char(&iS)[128] : ReplyBlock[iTheBlock].sentence)
        {
            // Find max sentences of this reply block
            if (iS[0] != '\0')
                iMax++;
        }

        // loop through all bots:
        for (int i = 1; i <= gpGlobals->maxClients; i++) {
            edict_t* pPlayer = INDEXENT(i);

            // skip invalid players and skip self (i.e. this bot)
            if (pPlayer && !pPlayer->free && pSender != pPlayer) {
                const bool bSenderAlive = IsAlive(pSender);      // CRASH : it sometimes crashes here
                const bool bPlayerAlive = IsAlive(pPlayer);

                if (bSenderAlive != bPlayerAlive)
                    continue;

                cBot* pBotPointer = UTIL_GetBotPointer(pPlayer);

                if (pBotPointer != nullptr)
                    if (RANDOM_LONG(0, 100) <
                        pBotPointer->ipChatRate + 25) {
                        // When we have at least 1 sentence...
                        if (iMax > -1) {
                            // choose randomly a reply
                            int the_c = RANDOM_LONG(0, iMax);

                            if (iTheBlock == iLastBlock &&
                                the_c == iLastSentence) {
                                // when this is the same, avoid it. Try to change again
                                if (iMax > 0)
                                    the_c++;
                                else
                                    continue;      // do not reply double

                                if (the_c > iMax)
                                    the_c = 0;
                            }
                            // the_c is choosen, it is the sentence we reply with.
                            // do a check if its valid:
                            if (ReplyBlock[iTheBlock].
                                sentence[the_c][0] != '\0') {

                                // chSentence is eventually what the bot will say.
                                char chSentence[128];
                                char temp[80];

                                std::memset(chSentence, 0, sizeof(chSentence));
                                std::memset(temp, 0, sizeof(temp));

                                // get character position
                                const char* name_pos =
                                    std::strstr(ReplyBlock[iTheBlock].
                                        sentence[the_c], "%n");

                                // when name_pos var is found, fill it in.
                                if (name_pos != nullptr) {
                                    // when name is in this one:
                                    int name_offset =
                                        name_pos -
                                        ReplyBlock[iTheBlock].sentence[the_c];
                                    name_offset--;

                                    // copy every character till name_offset
                                    int nC;
                                    for (nC = 0; nC < name_offset; nC++) {
                                        //chSentence[nC] = ReplyBlock[iTheBlock].sentence[the_c][nC];
                                        temp[nC] =
                                            ReplyBlock[iTheBlock].
                                            sentence[the_c][nC];
                                    }

                                    temp[nC] = ' ';

                                    // copy senders name to chSentence
                                    std::strcat(temp, sender);

                                    // Skip %n part in ReplyBlock
                                    nC = name_offset + 3;

                                    // we just copied a name to chSentence
                                    // set our cursor after the name now (name length + 1)
                                    int tc = static_cast<int>(std::strlen(temp));

                                    // now finish the sentence
                                    // get entire length of ReplyBlock and go until we reach the end
                                    const int length =
                                        static_cast<int>(std::strlen(ReplyBlock[iTheBlock].
                                            sentence[the_c]));


                                    // for every nC , read character from ReplyBlock
                                    for (; nC <= length; nC++) {
                                        // ... and copy it into chSentence
                                        temp[tc] =
                                            ReplyBlock[iTheBlock].
                                            sentence[the_c][nC];
                                        //char tmsg[80];
                                        //sprintf(tmsg,"Copying char %c , tc = %d, nC = %d\n", temp[tc], tc, nC);
                                        //SERVER_PRINT(tmsg);

                                        tc++;       // add up tc.
                                    }

                                    // terminate
                                    temp[tc] = '\n';

                                    snprintf(chSentence, sizeof(chSentence), "%s \n", temp);
                                }
                                // when no name pos is found, we just copy the string and say that (works ok)
                                else
                                    snprintf(chSentence, sizeof(chSentence), "%s \n",
                                        ReplyBlock[iTheBlock].sentence[the_c]);

                                // reply:
                                pBotPointer->PrepareChat(chSentence);

                                //UTIL_SayTextBot(chSentence, pBotPointer);

                                // update
                                iLastSentence = the_c;
                                iLastBlock = iTheBlock;
                            }
                        }
                    }
            }

        }

    }
    // clear sentence and such
    std::memset(sentence, 0, sizeof(sentence));
    std::memset(sender, 0, sizeof(sender));


    fThinkTimer = gpGlobals->time + RANDOM_FLOAT(0.0f, 0.5f);
}

void cChatEngine::handle_sentence() {
    // Define a constant for the chat rate threshold
    constexpr int CHAT_RATE_THRESHOLD = 25;

    // Check if there is a sentence to process
    if (sentence[0] == '\0') {
        return;
    }

    // Loop through all clients
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t* pPlayer = INDEXENT(i);

        // Skip invalid players
        if (pPlayer && !pPlayer->free) {
            cBot* pBotPointer = UTIL_GetBotPointer(pPlayer);

            // Check if bot pointer is valid and decide to prepare chat
            if (pBotPointer != nullptr && RANDOM_LONG(0, 100) < pBotPointer->ipChatRate + CHAT_RATE_THRESHOLD) {
                pBotPointer->PrepareChat(sentence);
            }
        }
    }

    // Clear sentence and sender buffers
    std::memset(sentence, 0, sizeof(sentence));
    std::memset(sender, 0, sizeof(sender));

    // Set the think timer
    fThinkTimer = gpGlobals->time + RANDOM_FLOAT(0.0f, 0.5f);
}


void cChatEngine::set_sentence(char csender[MAX_NAME_LENGTH], char csentence[MAX_SENTENCE_LENGTH]) {
    if (sender[0] == ' ' || sender[0] == '\0') {
        std::strcpy(sender, csender);
#ifdef _WIN32
        _strupr(csentence);
#else
        std::transform(csentence, csentence + std::strlen(csentence), csentence, ::toupper);
#endif
        std::strcpy(sentence, csentence);
    }
}


// $Log: ChatEngine.cpp,v $
// Revision 1.11  2004/09/07 15:44:34  eric
// - bumped build nr to 3060
// - minor changes in add2 (to add nodes for Bsp2Rbn utilities)
// - if compiled with USE_EVY_ADD, then the add2() function is used when adding
//   nodes based on human players instead of add()
// - else, it now compiles mostly without warnings :-)
//
// Revision 1.10  2004/07/05 19:15:54  eric
// - bumped to build 3052
// - modified the build_nr system to allow for easier build increment
// - a build.cpp file has been added and need to be incremented on each commit
// - some more flexibility for ChatEngine: ignore case and try to cope
//   with accute letters of French and Spanish and .. languages
//
