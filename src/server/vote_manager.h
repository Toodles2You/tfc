//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Voting manager
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include <string>
#include <vector>

class IPoll;
class CVoteManager;


class IPoll
{
public:
    static constexpr float kDuration = 20.0F;

private:
    float m_flStartTime;

    unsigned int m_IgnorePlayers;
    std::vector<int> m_Tally;

public:
    IPoll();
    ~IPoll();

protected:
    void Begin(const std::string& title, const std::vector<std::string>& options);

public:
    void ClientVote(unsigned int playerIndex, unsigned int option);

    bool IsDone();
    void End();

protected:
    virtual void DoResults(const int winner, const std::vector<int>& tally) = 0;
    virtual bool CanPlayerVote(CBasePlayer* player);
};


class CVoteManager
{
public:
    CVoteManager();
    ~CVoteManager();
    
    static void RegisterCvars();

    void Init();
    void Update();

	void ClientConnected(const unsigned int playerIndex);
    void ClientDisconnected(const unsigned int playerIndex);
	bool ClientCommand(const unsigned int playerIndex, const char* cmd);
    bool SayCommand(const unsigned int playerIndex, const int argc, const char** argv);

private:
    static constexpr float kUpdateInterval = 1;
    
    IPoll* m_Poll;
    float m_LastUpdateTime;

    void ClearPoll();

public:
    bool IsPollRunning() { return m_Poll != nullptr; }

private:
    enum class LevelVote
    {
        NotCalled,
        Called,
        ChangeImmediately,
    };

    LevelVote m_LevelVoteStatus;

public:
    struct LevelNominee
    {
        std::string name;
        unsigned int playerIndex;
    };

private:
    std::vector<LevelNominee> m_LevelNominees;
    unsigned int m_LevelChangeRequests;

    void LevelVoteUpdate();
    void LevelVoteBegin();

    void RequestLevelChange(const unsigned int playerIndex);
    void NominateLevel(const unsigned int playerIndex, const std::string& levelName);
};


class CLevelPoll : public IPoll
{
public:
    CLevelPoll(const std::vector<CVoteManager::LevelNominee>& nominees);

protected:
    void DoResults(const int winner, const std::vector<int>& tally) override;

private:
    std::vector<std::string> m_LevelNames;
};

