/* RequiredLibraries: jsoncpp */

/*
 * module { name = "m_github" }
 * github { channel = "#anope"; repos = "anope windows-libs" }
 */

#include "module.h"
#include "modules/httpd.h"

#include "jsoncpp/json/json.h"

struct GitHubChannel
{
	std::vector<Anope::string> repos;
	Anope::string channel;
};

std::vector<GitHubChannel> channels;

class GitHubPage : public HTTPPage
{
 public:
	GitHubPage() : HTTPPage("/github")
	{
	}

	bool OnRequest(HTTPProvider *server, const Anope::string &page_name, HTTPClient *client, HTTPMessage &message, HTTPReply &reply) anope_override
	{
		const Anope::string &payload = message.post_data["payload"];

		Json::Value root;
		Json::Reader reader;

		if (!reader.parse(payload.str(), root))
			return true;

		const Anope::string &repo_name = root["repository"]["name"].asString();
		Anope::string branch = root["ref"].asString();
		branch = branch.replace_all_cs("refs/heads/", "");

		for (unsigned i = 0; i < root["commits"].size(); ++i)
		{
			Json::Value &commit = root["commits"][i];

			const Anope::string &author_name = commit["author"]["name"].asString(),
				&commit_url = commit["url"].asString(),
				&commit_msg = commit["message"].asString();

			std::vector<Anope::string> commit_message;
			if (commit_msg.find("\n") != Anope::string::npos)
			{
				commit_message.push_back("\2" + repo_name + "\2: \00303" + author_name + "\003 \00307" + branch + "\003: " + commit_url);

				sepstream sep(commit_msg, '\n');
				Anope::string token;
				while (sep.GetToken(token))
					commit_message.push_back(token);
			}
			else
				commit_message.push_back("\2" + repo_name + "\2: \00303" + author_name + "\003 \00307" + branch + "\003: " + commit_msg + " | " + commit_url);

			for (unsigned j = 0; j < channels.size(); ++j)
			{
				GitHubChannel &chan = channels[j];

				if (std::find(chan.repos.begin(), chan.repos.end(), repo_name) == chan.repos.end())
					continue;

				Channel *c = Channel::Find(chan.channel);
				
				if (c && c->ci && c->ci->bi)
					for (unsigned k = 0; k < commit_message.size(); ++k)
						IRCD->SendPrivmsg(c->ci->bi, c->name, "%s", commit_message[k].c_str());
			}
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

		ServiceReference<HTTPProvider> provider("HTTPProvider", block->Get<const Anope::string>("server", "httpd/main"));
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
		Configuration::Block *config = Config->GetModule(this);

		for (int i = 0; i < config->CountBlock("github"); ++i)
		{
			Configuration::Block *block = config->GetBlock("github", i);
			GitHubChannel chan;
			chan.channel = block->Get<const Anope::string>("channel");
			spacesepstream(block->Get<const Anope::string>("repos")).GetTokens(chan.repos);
			channels.push_back(chan);
		}
	}
};

MODULE_INIT(GitHub)
