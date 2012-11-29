/* RequiredLibraries: json */

#include "module.h"
#include "../extra/httpd.h"

#include "json/json.h"

static std::vector<Anope::string> channels;

class WebPanelPage : public HTTPPage
{
 public:
	WebPanelPage() : HTTPPage("/github")
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

			Anope::string commit_message = "\2" + repo_name + "\2: \00303" + author_name + "\003 \00307" + branch + "\003: " + commit_msg + " | " + commit_url;

			for (unsigned j = 0; j < channels.size(); ++j)
			{
				Channel *c = Channel::Find(channels[j]);
				
				if (c && c->ci && c->ci->bi)
					IRCD->SendPrivmsg(c->ci->bi, c->name, "%s", commit_message.c_str());
			}
		}

		return true;
	}
};

class GitHub : public Module
{
	ServiceReference<HTTPProvider> provider;
	WebPanelPage github_page;

 public:
	GitHub(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator),
		provider("HTTPProvider", "httpd/main")
	{
		this->SetAuthor("Adam");

		ConfigReader reader;
		Anope::string provider_name = reader.ReadValue("github", "server", "httpd/main", 0);

		provider = provider_name;
		if (!provider)
			throw ModuleException("Unable to find HTTPD provider. Is m_httpd loaded?");

		provider->RegisterPage(&this->github_page);

		for (int i = 0; i < reader.Enumerate("github"); ++i)
			channels.push_back(reader.ReadValue("github", "channel", i));
	}

	~GitHub()
	{
		if (provider)
			provider->UnregisterPage(&this->github_page);
	}
};

MODULE_INIT(GitHub)
