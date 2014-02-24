/* RequiredLibraries: jsoncpp */

/*
 * module
 * {
 * 	name = "m_github"
 * 	github
 * 	{
 * 		channel = "#anope";
 * 		repos = "anope anotherreop";
 * 		events = "issues issue_comment watch pull_request pull_request_review_comment push commit_comment release fork"
 * 	}
 * }
 */

#include "module.h"
#include "modules/httpd.h"

#include "jsoncpp/json/json.h"

struct GitHubChannel
{
	std::vector<Anope::string> repos;
	std::vector<Anope::string> events;
	Anope::string channel;
};

static std::vector<GitHubChannel> channels;

class GitHubPage : public HTTPPage
{
	std::vector<Anope::string> starred;

 public:
	GitHubPage() : HTTPPage("/github")
	{
	}

 private:
	Anope::string Bold(const Anope::string &text) const
	{
		return "\2" + text + "\2";
	}

	Anope::string Green(const Anope::string &text) const
	{
		return "\00303" + text + "\003";
	}

	Anope::string Orange(const Anope::string &text) const
	{
		return "\00307" + text + "\003";
	}

	void HandleIssueComment(const Json::Value &root, std::vector<Anope::string> &lines)
	{
		const std::string& reponame = root["repository"]["name"].asString();
		lines.push_back(Bold(reponame) + ": " + Green(root["sender"]["login"].asString())
			+ " commented on issue #" + stringify(root["issue"]["number"].asUInt()) + ": " + root["issue"]["title"].asString()
			+ " - Link: " + root["issue"]["html_url"].asString());

		const std::string& body = root["comment"]["body"].asString();

		sepstream sep(body, '\n');
		for (Anope::string token; sep.GetToken(token);)
			lines.push_back(Bold(reponame) + ": " + token);
	}

	void HandleIssues(const Json::Value &root, std::vector<Anope::string> &lines)
	{
		lines.push_back(Bold(root["repository"]["name"].asString()) + ": " + Green(root["sender"]["login"].asString())
			+ " " + root["action"].asString() + " issue #" + Green(stringify(root["issue"]["number"].asUInt()))
			+ ": " + root["issue"]["title"].asString() + " - Link: " + root["issue"]["html_url"].asString());
	}

	void HandleWatch(const Json::Value &root, std::vector<Anope::string> &lines)
	{
		if (starred.size() > 25)
			starred.resize(25);

		const Anope::string& user = root["sender"]["login"].asString();

		if (std::find(starred.begin(), starred.end(), user) != starred.end())
			return;

		starred.insert(starred.begin(), user);

		lines.push_back(Bold(root["repository"]["name"].asString()) + ": " + Green(user)
			+ " starred the project!");
	}

	void HandlePullRequest(const Json::Value &root, std::vector<Anope::string> &lines)
	{
		std::string action = (root["action"].asString() == "synchronize" ? "rebased" : root["action"].asString());

		lines.push_back(Bold(root["repository"]["name"].asString()) + ": " + Green(root["sender"]["login"].asString())
			+ " " + action + " PR #" + stringify(root["number"].asUInt()) + " on "
			+ Orange(root["pull_request"]["base"]["ref"].asString()) + ": "
			+ root["pull_request"]["title"].asString() + " - Link: " + root["pull_request"]["html_url"].asString());
	}

	void HandlePullRequestComment(const Json::Value &root, std::vector<Anope::string> &lines)
	{
		Anope::string pr = root["comment"]["pull_request_url"].asString();
		size_t pos = pr.find_last_of('/');
		if (pr.length() < 2 || pos == Anope::string::npos)
			return;
		pr = pr.substr(pos + 1);

		const std::string& reponame = root["repository"]["name"].asString();
		lines.push_back(Bold(reponame) + ": " + Green(root["sender"]["login"].asString())
			+ " commented on PR #" + pr + " - Link: " + root["comment"]["html_url"].asString());

		const std::string& body = root["comment"]["body"].asString();

		sepstream sep(body, '\n');
		for (Anope::string token; sep.GetToken(token);)
			lines.push_back(Bold(reponame) + ": " + token);
	}

	void HandleCommitComment(const Json::Value &root, std::vector<Anope::string> &lines)
	{
		lines.push_back(Bold(root["repository"]["name"].asString()) + ": " + Green(root["sender"]["login"].asString())
			+ " commented on commit " + root["comment"]["commit_id"].asString() + " - Link: " + root["comment"]["html_url"].asString());
	}

	void HandleRelease(const Json::Value &root, std::vector<Anope::string> &lines)
	{
		lines.push_back(Bold(root["repository"]["name"].asString()) + ": " + Green(root["release"]["author"]["login"].asString())
			+ " has just " + Orange(root["action"].asString()) + " " + Green(root["release"]["name"].asString()) + "!");
	}

	void HandleFork(const Json::Value &root, std::vector<Anope::string> &lines)
	{
		lines.push_back(Bold(root["repository"]["name"].asString()) + " was just forked by "
			+ Green(root["forkee"]["owner"]["login"].asString()) + " - Link: "
			+ root["forkee"]["html_url"].asString());
	}

	void HandlePush(const Json::Value &root, std::vector<Anope::string> &commit_message)
	{
		const Anope::string &repo_name = root["repository"]["name"].asString();
		Anope::string branch = root["ref"].asString();
		branch = branch.replace_all_cs("refs/heads/", "");

		for (unsigned i = 0; i < root["commits"].size(); ++i)
		{
			const Json::Value &commit = root["commits"][i];

			const Anope::string &author_name = commit["author"]["name"].asString(),
				&commit_url = commit["url"].asString(),
				&commit_msg = commit["message"].asString();

			if (commit_msg.find("\n") != Anope::string::npos)
			{
				commit_message.push_back(Bold(repo_name) + ": " + Green(author_name) + " " + Orange(branch) + ": " + commit_url);

				sepstream sep(commit_msg, '\n');
				for (Anope::string token; sep.GetToken(token);)
					commit_message.push_back(token);
			}
			else
				commit_message.push_back(Bold(repo_name) + ": " + Green(author_name) + " " + Orange(branch) + ": " + commit_msg + " | " + commit_url);
		}
	}

	bool OnRequest(HTTPProvider *server, const Anope::string &page_name, HTTPClient *client, HTTPMessage &message, HTTPReply &reply) anope_override
	{
		const Anope::string &payload = message.post_data["payload"];

		Json::Value root;
		Json::Reader reader;

		if (!reader.parse(payload.str(), root))
			return true;

		Anope::string event = message.headers["X-GitHub-Event"];
		std::vector<Anope::string> lines;
		if (event == "issue_comment")
			HandleIssueComment(root, lines);
		else if (event == "issues")
			HandleIssues(root, lines);
		else if (event == "watch")
			HandleWatch(root, lines);
		else if (event == "pull_request")
			HandlePullRequest(root, lines);
		else if (event == "pull_request_review_comment")
			HandlePullRequestComment(root, lines);
		else if (event == "push")
			HandlePush(root, lines);
		else if (event == "commit_comment")
			HandleCommitComment(root, lines);
		else if (event == "release")
			HandleRelease(root, lines);
		else if (event == "fork")
			HandleFork(root, lines);

		const Anope::string &repo_name = root["repository"]["name"].asString();

		for (std::vector<GitHubChannel>::iterator chan = channels.begin(); chan != channels.end(); chan++)
		{
			if (std::find(chan->repos.begin(), chan->repos.end(), repo_name) == chan->repos.end())
				continue;

			if (!chan->events.empty())
				if (std::find(chan->events.begin(), chan->events.end(), event) == chan->events.end())
					continue;			

			Channel *c = Channel::Find(chan->channel);
			if (!c || !c->ci || !c->ci->bi)
				continue;

			for (std::vector<Anope::string>::iterator it = lines.begin(); it != lines.end(); it++)
				IRCD->SendPrivmsg(*c->ci->bi, c->name, "%s", it->c_str());
		}

		return true;
	}
};

class GitHub : public Module
{
	ServiceReference<HTTPProvider> provider;
	GitHubPage github_page;

 public:
	GitHub(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD)
	{
		this->SetAuthor("Adam");

		Configuration::Block *block = Config->GetModule(this);
		provider = ServiceReference<HTTPProvider>("HTTPProvider", block->Get<const Anope::string>("server", "httpd/main"));
		if (!provider)
			throw ModuleException("Unable to find HTTPD provider. Is m_httpd loaded?");

		provider->RegisterPage(&this->github_page);
	}

	~GitHub()
	{
		if (provider)
			provider->UnregisterPage(&this->github_page);
	}

	void OnReload(Configuration::Conf *conf) anope_override
	{
		channels.clear();
		Configuration::Block *config = conf->GetModule(this);

		for (int i = 0; i < config->CountBlock("github"); ++i)
		{
			Configuration::Block *block = config->GetBlock("github", i);
			GitHubChannel chan;
			chan.channel = block->Get<Anope::string>("channel");
			spacesepstream(block->Get<Anope::string>("repos")).GetTokens(chan.repos);
			spacesepstream(block->Get<Anope::string>("events")).GetTokens(chan.events);
			channels.push_back(chan);
		}
	}
};

MODULE_INIT(GitHub)
