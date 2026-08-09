/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryStore.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "QueryResult.h"
#include <sstream>

namespace
{
    // IGNORE skips arena_team_member rows whose team no longer exists. mod-1v1-arena leaves such
    // orphans behind (it deletes from arena_team without touching arena_team_member) and they would
    // otherwise abort the statement on a foreign key violation.
    constexpr char const* UPSERT_PREFIX =
        "INSERT IGNORE INTO mod_arena_rating_memory (guid, arenaTeamId, personalRating) VALUES ";

    constexpr char const* UPSERT_SUFFIX =
        " ON DUPLICATE KEY UPDATE personalRating = VALUES(personalRating)";
}

void ArenaRatingMemory::SeedFromExistingTeams()
{
    CharacterDatabase.Execute(
        "INSERT IGNORE INTO mod_arena_rating_memory (guid, arenaTeamId, personalRating) "
        "SELECT guid, arenaTeamId, personalRating FROM arena_team_member "
        "ON DUPLICATE KEY UPDATE personalRating = VALUES(personalRating)");
}

void ArenaRatingMemory::RememberTeam(uint32 arenaTeamId, std::vector<MemberRating> const& members)
{
    if (members.empty())
        return;

    // One statement per team rather than per member: ArenaTeamMgr::DistributeArenaPoints saves
    // every arena team on the server in a single weekly sweep.
    std::ostringstream query;
    query << UPSERT_PREFIX;

    bool first = true;
    for (MemberRating const& member : members)
    {
        if (!first)
            query << ',';
        first = false;

        query << '(' << member.Guid.GetCounter() << ',' << arenaTeamId << ',' << member.PersonalRating << ')';
    }

    query << UPSERT_SUFFIX;
    CharacterDatabase.Execute(query.str());
}

Optional<uint32> ArenaRatingMemory::Recall(uint32 arenaTeamId, ObjectGuid playerGuid)
{
    QueryResult result = CharacterDatabase.Query(
        "SELECT personalRating FROM mod_arena_rating_memory WHERE guid = {} AND arenaTeamId = {}",
        playerGuid.GetCounter(), arenaTeamId);

    if (!result)
        return std::nullopt;

    return result->Fetch()[0].Get<uint32>();
}

std::vector<ArenaRatingMemory::RememberedTeam> ArenaRatingMemory::RecallAll(ObjectGuid playerGuid)
{
    std::vector<RememberedTeam> remembered;

    // The foreign key guarantees the team still exists, so the join always matches.
    QueryResult result = CharacterDatabase.Query(
        "SELECT m.arenaTeamId, t.name, m.personalRating, m.updatedAt "
        "FROM mod_arena_rating_memory m JOIN arena_team t ON t.arenaTeamId = m.arenaTeamId "
        "WHERE m.guid = {} ORDER BY t.name",
        playerGuid.GetCounter());

    if (!result)
        return remembered;

    do
    {
        Field* fields = result->Fetch();
        remembered.push_back({
            fields[0].Get<uint32>(),
            fields[1].Get<std::string>(),
            fields[2].Get<uint32>(),
            fields[3].Get<std::string>()
        });
    } while (result->NextRow());

    return remembered;
}

void ArenaRatingMemory::Forget(ObjectGuid playerGuid, Optional<uint32> arenaTeamId)
{
    if (arenaTeamId)
        CharacterDatabase.Execute("DELETE FROM mod_arena_rating_memory WHERE guid = {} AND arenaTeamId = {}",
            playerGuid.GetCounter(), *arenaTeamId);
    else
        CharacterDatabase.Execute("DELETE FROM mod_arena_rating_memory WHERE guid = {}", playerGuid.GetCounter());
}
