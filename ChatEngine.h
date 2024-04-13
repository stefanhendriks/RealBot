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
#ifndef CHATENGINE_H
#define CHATENGINE_H

#define MAX_BLOCKS	100
#define BLOCK_DEATHS (MAX_BLOCKS-1)

static constexpr int MAX_SENTENCE_LENGTH = 128;
// Reply block
typedef struct {
   // words, hinting that in this block a logical sentence will be to reply with
   char word[10][25];
   char sentence[50][MAX_SENTENCE_LENGTH];      // at max 50 sentences of 80 characters to reply with
   bool bUsed;
}
tReplyBlock;

class cChatEngine {
public:
   // variables
   tReplyBlock ReplyBlock[MAX_BLOCKS];  // 100 reply blocks reserved in memory
   float fThinkTimer;                   // The chatengine has a 'think timer'.

   char sender[30];
   char sentence[MAX_SENTENCE_LENGTH];

   int iLastBlock;
   int iLastSentence;

   // functions
   void init();                 // initialize
   void initAndload();          // initialize and load data from file
   void think();                // make the chat engine think

   // add sentence from any player/bot into memory to handle
   void set_sentence(char csender[30], char csentence[MAX_SENTENCE_LENGTH]);

   // handles a sentence, decides to reply on it or not.
   void handle_sentence();
};

#endif // CHATENGINE_H