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

// COUNTER-STRIKE

#ifndef BOT_CLIENT_H
#define BOT_CLIENT_H

void BotClient_CS_VGUI(void *p, int bot_index);
void BotClient_CS_ShowMenu(void *p, int bot_index);
void BotClient_CS_WeaponList(void *p, int bot_index);
void BotClient_CS_CurrentWeapon(void *p, int bot_index);
void BotClient_CS_AmmoX(void *p, int bot_index);
void BotClient_CS_AmmoPickup(void *p, int bot_index);
void BotClient_CS_WeaponPickup(void *p, int bot_index);
void BotClient_CS_ItemPickup(void *p, int bot_index);
void BotClient_CS_Health(void *p, int bot_index);
void BotClient_CS_Battery(void *p, int bot_index);
void BotClient_CS_Damage(void *p, int bot_index);
void BotClient_CS_Money(void *p, int bot_index);
void BotClient_CS_DeathMsg(void *p, int bot_index);
void BotClient_CS_ScreenFade(void *p, int bot_index);
void BotClient_CS_HLTV(void *p, int bot_index);
void BotClient_CS_SayText(void *p, int bot_index);

// StatusIcon
void BotClient_CS_StatusIcon(void *p, int bot_index);

// VALVE DEATHMATCH
void BotClient_Valve_WeaponList(void *p, int bot_index);
void BotClient_Valve_CurrentWeapon(void *p, int bot_index);
void BotClient_Valve_AmmoX(void *p, int bot_index);
void BotClient_Valve_AmmoPickup(void *p, int bot_index);
void BotClient_Valve_WeaponPickup(void *p, int bot_index);
void BotClient_Valve_ItemPickup(void *p, int bot_index);
void BotClient_Valve_Health(void *p, int bot_index);
void BotClient_Valve_Battery(void *p, int bot_index);
void BotClient_Valve_Damage(void *p, int bot_index);
void BotClient_Valve_DeathMsg(void *p, int bot_index);
void BotClient_Valve_ScreenFade(void *p, int bot_index);

#endif // BOT_CLIENT_H