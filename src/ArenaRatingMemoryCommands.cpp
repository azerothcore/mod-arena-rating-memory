/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryStore.h"
#include "Chat.h"
#include "CommandScript.h"
#include "Player.h"

using namespace Acore::ChatCommands;

// Registered as "arena ratingmemory" so it nests under the core's existing .arena command instead
// of adding a top level one; command names are split on spaces and intermediate nodes are reused.
class ArenaRatingMemoryCommandScript : public CommandScript
{
public:
    ArenaRatingMemoryCommandScript() : CommandScript("ArenaRatingMemoryCommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable ratingMemoryTable =
        {
            { "arena ratingmemory show",  HandleShowCommand,  SEC_GAMEMASTER,     Console::Yes },
            { "arena ratingmemory clear", HandleClearCommand, SEC_ADMINISTRATOR,  Console::Yes }
        };

        return ratingMemoryTable;
    }

    static bool HandleShowCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!target)
            return false;

        std::vector<ArenaRatingMemory::RememberedTeam> const remembered =
            ArenaRatingMemory::RecallAll(target->GetGUID());

        if (remembered.empty())
        {
            handler->PSendSysMessage("No remembered arena ratings for {}.", target->GetName());
            return true;
        }

        handler->PSendSysMessage("Remembered arena ratings for {}:", target->GetName());

        for (ArenaRatingMemory::RememberedTeam const& entry : remembered)
            handler->PSendSysMessage("  [{}] {} - rating {}, week {}/{}, season {}/{} (updated {})",
                entry.ArenaTeamId, entry.TeamName, entry.PersonalRating, entry.WeekGames, entry.WeekWins,
                entry.SeasonGames, entry.SeasonWins, entry.UpdatedAt);

        return true;
    }

    static bool HandleClearCommand(ChatHandler* handler, Optional<PlayerIdentifier> target,
        Optional<uint32> arenaTeamId)
    {
        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!target)
            return false;

        std::size_t const affected = arenaTeamId
            ? (ArenaRatingMemory::Recall(*arenaTeamId, target->GetGUID()) ? 1u : 0u)
            : ArenaRatingMemory::RecallAll(target->GetGUID()).size();

        if (!affected)
        {
            handler->PSendSysMessage("Nothing to clear for {}.", target->GetName());
            return true;
        }

        ArenaRatingMemory::Forget(target->GetGUID(), arenaTeamId);

        // Only the module's own table is touched. A player still in the team keeps their live
        // rating, and the next arena match stores it again.
        handler->PSendSysMessage("Cleared {} remembered arena rating(s) for {}.", affected, target->GetName());

        if (!arenaTeamId)
            handler->SendSysMessage("Teams they still belong to will be recorded again after their next match.");

        return true;
    }
};

void AddArenaRatingMemoryCommandScripts()
{
    new ArenaRatingMemoryCommandScript();
}
