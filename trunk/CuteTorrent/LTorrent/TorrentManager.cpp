#include "TorrentManager.h"
#include <QMessageBox>
#include  <QFile>
#include  <QDir>
#include  <QStringList>
#include <QTextStream>
#include <QDebug>
using namespace libtorrent;

TorrentManager::TorrentManager()
{

	
	torrentSettings = QApplicationSettings::getInstance();
	
	ses = new session(fingerprint("CuteTorrent", VERSION_MAJOR ,VERSION_MINOR,VERSION_MINCHANGES ,VERSION_MIZER)
		, session::add_default_plugins
		, alert::all_categories
			& ~(alert::dht_notification
			+ alert::progress_notification
			+ alert::debug_notification
			+ alert::stats_notification));
	error_code ec;
	std::vector<char> in;
	if (load_file("CT_DATA/actual.state", in, ec) == 0)
	{
		lazy_entry e;
		if (lazy_bdecode(&in[0], &in[0] + in.size(), e, ec) == 0)
			ses->load_state(e);
	}
	session_settings s_settings=readSettings();
	// local perrs serach
	ses->start_lsd();
	// upnp
	ses->start_upnp();
	ses->start_natpmp();

	create_directory("CT_DATA",ec);
	
	std::string bind_to_interface = "";
	
	ses->listen_on(std::make_pair(listen_port, listen_port)
		, ec, bind_to_interface.c_str());
	if (ec)
	{
		QMessageBox msg;
		msg.setText("Failed to bing port "+listen_port);
		msg.exec();
		//fprintf(stderr, "failed to listen on %s on ports %d-%d: %s\n", bind_to_interface.c_str(), listen_port, listen_port+1, ec.message().c_str());
	}




	s_settings.use_dht_as_fallback = false;

		ses->add_dht_router(std::make_pair(
			std::string("router.bittorrent.com"), 6881));
		ses->add_dht_router(std::make_pair(
			std::string("router.utorrent.com"), 6881));
		ses->add_dht_router(std::make_pair(
			std::string("router.bitcomet.com"), 6881));

		ses->start_dht();

	ses->set_settings(s_settings);
	qDebug() << "TorrentManager: intialisation completed";
	qDebug() << "TorrentManager: session restored";
/*	ses->load_asnum_db("GeoIPASNum.dat");
	ses->load_country_db("GeoIP.dat");*/
}

void TorrentManager::initSession()
{
	QDir dir("CT_DATA");
	QStringList filter;
	error_code ec;
	filter <<"*.torrent";
	QStringList torrentFiles=dir.entryList(filter);
	QFile path_infohashFile("CT_DATA/path.resume");
	if (path_infohashFile.open(QFile::ReadOnly))
	{
		QTextStream strm(&path_infohashFile);
		strm.setCodec("UTF-8");
		QString line;
		while(!strm.atEnd())
		{
			line=strm.readLine();
			QStringList parts=line.split("|");
			if (parts.count()>2)
				continue;
			
			save_path_data.insert(parts.at(0),parts.at(1));
		}
		path_infohashFile.close();
	}
	else
	{
		QMessageBox::warning(NULL,"Warning",QString::fromLocal8Bit("�� ������� ������� ���� CT_DATA/path.resume!\n���� �� ���������� ����������� ������ ��� ������ ������� Ok."));
			
			
	}
	for (QStringList::iterator i=torrentFiles.begin();i!=torrentFiles.end();i++)
	{
		
		torrent_info* t= new torrent_info(dir.filePath(*i).toAscii().data(),ec);
		if (!ec)
		if (save_path_data.contains(to_hex(t->info_hash().to_string()).c_str()))
		{
			
			AddTorrent(dir.filePath(*i),save_path_data[to_hex(t->info_hash().to_string()).c_str()]);
		}

	}
}
bool yes2(libtorrent::torrent_status const&)
{return true;}
bool yes(libtorrent::torrent_status const&)
{ return true; }
std::vector<torrent_status> TorrentManager::GetTorrents()
{
	std::vector<torrent_status> result;
	
	
	ses->get_torrent_status(&result,yes2);
		
	return result;
}
void TorrentManager::PostTorrentUpdate()
{
/*	std::deque<alert*> alerts;
		ses->pop_alerts(&alerts);
		std::string now = time_now_string();
		for (std::deque<alert*>::iterator i = alerts.begin()
			, end(alerts.end()); i != end; ++i)
		{
			bool need_resort = false;
			TORRENT_TRY
			{
				
					// if we didn't handle the alert, print it to the log
					
					QMessageBox::warning(0,"",QString::fromLocal8Bit((*i)->message().c_str()));
					
				
			} TORRENT_CATCH(std::exception& e) {}

			



			delete *i;
		}
		alerts.clear();*/
	ses->post_torrent_updates();
}
bool TorrentManager::AddTorrent(QString path, QString save_path)
{
	if (path.startsWith("http://") || path.startsWith("udp://") || path.startsWith("https://") || path.startsWith("magnet:") )
	{
		add_torrent_params p;
		p.save_path = std::string(save_path.toAscii().data());
		p.storage_mode = storage_mode_sparse;
		p.url = std::string(path.toAscii().data());
		std::vector<char> buf;
		error_code ec;
		if (path.startsWith("magnet:"))
		{
			add_torrent_params tmp;
			parse_magnet_uri(std::string(path.toAscii().data()), tmp, ec);
			
			std::string filename = combine_path(std::string(save_path.toAscii().data()), combine_path(".fastresume"
				, to_hex(tmp.info_hash.to_string()) + ".fastresume"));
			if (load_file(filename.c_str(), buf, ec) == 0)
				p.resume_data = &buf;
		}
		ses->async_add_torrent(p);
		
	}
	else
	{
		
		boost::intrusive_ptr<torrent_info> t;
		error_code ec;
		
		
		t = new torrent_info(path.toAscii().data(), ec);
		
		if (ec)
		{
			QMessageBox msg;
			msg.setText(ec.message().c_str());
			msg.exec();
			return false;
		}
	
		

		add_torrent_params p;
		
		lazy_entry resume_data;

		std::string filename = combine_path("CT_DATA", to_hex(t->info_hash().to_string()) + ".resume");

		std::vector<char> buf;
		if (load_file(filename.c_str(), buf, ec) == 0)
		{
			p.resume_data = &buf;
		}
		
		p.ti = t;
		
		p.save_path = std::string(save_path.toUtf8().data());
		p.storage_mode = storage_mode_sparse;
		p.flags |= add_torrent_params::flag_paused;
		p.flags |= add_torrent_params::flag_duplicate_is_error;
		p.userdata = (void*)strdup(path.toAscii().data());
		torrent_handle h=ses->add_torrent(p,ec);
		
	
		
		if (ec)
		{
			QMessageBox::warning(0,"Error",ec.message().c_str());
			return false;
		}
		emit AddTorrentGui(new Torrent(h.status()));
		QFileInfo file(path);
		h.set_max_connections(max_connections_per_torrent);
		QFile::copy(path,combine_path(QDir::currentPath().toAscii().data(),combine_path("CT_DATA",file.fileName().toAscii().data()).c_str()).c_str());
		if (save_path_data.contains(to_hex(h.info_hash().to_string()).c_str()))
		{
			save_path_data[to_hex(h.info_hash().to_string()).c_str()]=save_path.toUtf8().data();
		}
		else
		{
			save_path_data.insert(to_hex(h.info_hash().to_string()).c_str(),save_path.toUtf8().data());
		}

	}
	return true;
}
session_settings TorrentManager::readSettings()
{

	session_settings s_settings=ses->settings();
	
	QString baseDir=torrentSettings->valueString("System","BaseDir");
	if (baseDir.isEmpty())
	{
		baseDir=QDir::currentPath();
		torrentSettings->setValue("System","BaseDir",baseDir);
	}
	QDir::setCurrent(QDir::toNativeSeparators (baseDir));
	s_settings.half_open_limit = torrentSettings->valueInt("Torrent","half_open_limit",0x7fffffff);
	s_settings.allow_multiple_connections_per_ip = torrentSettings->valueBool("Torrent","allow_multiple_connections_per_ip",true);
	listen_port=torrentSettings->valueInt("Torrent","listen_port",6103);
	s_settings.use_disk_read_ahead = torrentSettings->valueBool("Torrent","use_disk_read_ahead",true);
	s_settings.disable_hash_checks = torrentSettings->valueBool("Torrent","disable_hash_checks",false);;
	s_settings.peer_timeout = torrentSettings->valueInt("Torrent","peer_timeout",120);
	s_settings.announce_to_all_tiers = torrentSettings->valueBool("Torrent","announce_to_all_tiers",true);
	s_settings.download_rate_limit = torrentSettings->valueInt("Torrent","download_rate_limit",0);
	s_settings.upload_rate_limit = torrentSettings->valueInt("Torrent","upload_rate_limit",0);

	s_settings.unchoke_slots_limit = torrentSettings->valueInt("Torrent","unchoke_slots_limit",8);
	s_settings.urlseed_wait_retry = torrentSettings->valueInt("Torrent","urlseed_wait_retry",30);
	s_settings.listen_queue_size = torrentSettings->valueInt("Torrent","listen_queue_size",30);

	s_settings.max_peerlist_size = torrentSettings->valueInt("Torrent","max_peerlist_size",4000);
	s_settings.max_paused_peerlist_size = torrentSettings->valueInt("Torrent","max_paused_peerlist_size",4000);
	ipFilterFileName = torrentSettings->valueString("Torrent","ip_filter_filename","");
	FILE* filter = fopen(ipFilterFileName.toAscii().data(), "r");
	if (filter)
	{
		ip_filter fil;
		unsigned int a,b,c,d,e,f,g,h, flags;
		while (fscanf(filter, "%u.%u.%u.%u - %u.%u.%u.%u %u\n", &a, &b, &c, &d, &e, &f, &g, &h, &flags) == 9)
		{
			address_v4 start((a << 24) + (b << 16) + (c << 8) + d);
			address_v4 last((e << 24) + (f << 16) + (g << 8) + h);
			if (flags <= 127) flags = ip_filter::blocked;
			else flags = 0;
			fil.add_rule(start, last, flags);
		}
		ses->set_ip_filter(fil);
		fclose(filter);
	}
	max_connections_per_torrent = torrentSettings->valueInt("Torrent","max_connections_per_torrent",50);
	useProxy = torrentSettings->valueBool("Torrent","useProxy",false);
	if (useProxy)
	{
		ps.hostname = torrentSettings->valueString("Torrent","proxy_hostname").toUtf8().constData();
		ps.port = torrentSettings->valueInt("Torrent","proxy_port");
		ps.type = (proxy_settings::proxy_type)torrentSettings->valueInt("Torrent","proxy_type");
		ps.username = torrentSettings->valueString("Torrent","proxy_username").toUtf8().constData();
		ps.password = torrentSettings->valueString("Torrent","proxy_password").toUtf8().constData();
		ses->set_proxy(ps);
	}
	s_settings.cache_size = torrentSettings->valueInt("Torrent","cache_size",2048);
	s_settings.use_read_cache =  torrentSettings->valueInt("Torrent","use_read_cache",s_settings.cache_size > 0);
	s_settings.cache_buffer_chunk_size = torrentSettings->valueInt("Torrent","cache_buffer_chunk_size",s_settings.cache_size /100);
	s_settings.allowed_fast_set_size = torrentSettings->valueInt("Torrent","allowed_fast_set_size",10);
	s_settings.read_cache_line_size = torrentSettings->valueInt("Torrent","read_cache_line_size",40);
	s_settings.allow_reordered_disk_operations = torrentSettings->valueBool("Torrent","allow_reordered_disk_operations",true);
	s_settings.active_downloads = torrentSettings->valueInt("Torrent","active_downloads",3);
	s_settings.active_limit = torrentSettings->valueInt("Torrent","active_limit",(std::max)(s_settings.active_downloads * 2, s_settings.active_limit));
	s_settings.active_seeds = torrentSettings->valueInt("Torrent","active_seeds",5);
	s_settings.choking_algorithm = session_settings::auto_expand_choker;
	s_settings.disk_cache_algorithm = session_settings::avoid_readback;
	s_settings.volatile_read_cache = false;
	s_settings.allow_reordered_disk_operations = false;
	DTInstallPath = torrentSettings->valueString("DT","DTInstallPath");
	
	return s_settings;
}
void TorrentManager::updateSettings(libtorrent::session_settings settings)
{
	ses->set_settings(settings);
	
	

}
void TorrentManager::writeSettings()
{
	session_settings s_settings=ses->settings();

	torrentSettings->setValue("Torrent","half_open_limit",s_settings.half_open_limit);
	torrentSettings->setValue("Torrent","allow_multiple_connections_per_ip",s_settings.allow_multiple_connections_per_ip);
	
	torrentSettings->setValue("Torrent","use_disk_read_ahead",s_settings.use_disk_read_ahead);
	torrentSettings->setValue("Torrent","disable_hash_checks",s_settings.disable_hash_checks);;
	torrentSettings->setValue("Torrent","peer_timeout",s_settings.peer_timeout);
	torrentSettings->setValue("Torrent","announce_to_all_tiers",s_settings.announce_to_all_tiers);
	torrentSettings->setValue("Torrent","download_rate_limit",s_settings.download_rate_limit);
	torrentSettings->setValue("Torrent","upload_rate_limit",s_settings.upload_rate_limit);

	torrentSettings->setValue("Torrent","unchoke_slots_limit",s_settings.unchoke_slots_limit);
	torrentSettings->setValue("Torrent","urlseed_wait_retry",s_settings.urlseed_wait_retry);
	torrentSettings->setValue("Torrent","listen_queue_size",s_settings.listen_queue_size);

	torrentSettings->setValue("Torrent","max_peerlist_size",s_settings.max_peerlist_size);
	torrentSettings->setValue("Torrent","max_paused_peerlist_size",s_settings.max_paused_peerlist_size);
	torrentSettings->setValue("Torrent","ip_filter_filename",ipFilterFileName);
	torrentSettings->setValue("Torrent","max_connections_per_torrent",max_connections_per_torrent);
	torrentSettings->setValue("Torrent","useProxy",useProxy);
	
		torrentSettings->setValue("Torrent","proxy_hostname",ps.hostname.c_str());
		torrentSettings->setValue("Torrent","proxy_port",ps.port);
		torrentSettings->setValue("Torrent","proxy_type",ps.type);
		torrentSettings->setValue("Torrent","proxy_username",ps.username.c_str());
		torrentSettings->setValue("Torrent","proxy_password",ps.password.c_str());
		
	
	torrentSettings->setValue("Torrent","cache_size",s_settings.cache_size);
	torrentSettings->setValue("Torrent","use_read_cache",s_settings.use_read_cache);
	torrentSettings->setValue("Torrent","cache_buffer_chunk_size",s_settings.cache_buffer_chunk_size);
	torrentSettings->setValue("Torrent","allowed_fast_set_size",s_settings.allowed_fast_set_size);
	torrentSettings->setValue("Torrent","read_cache_line_size",s_settings.read_cache_line_size);
	torrentSettings->setValue("Torrent","allow_reordered_disk_operations",s_settings.allow_reordered_disk_operations);
	torrentSettings->setValue("Torrent","active_downloads",s_settings.active_downloads);
	torrentSettings->setValue("Torrent","active_limit",s_settings.active_limit);
	torrentSettings->setValue("Torrent","active_seeds",s_settings.active_seeds);
}
void TorrentManager::onClose()
{
	qDebug() << "start saving session";
	writeSettings();
	int num_outstanding_resume_data = 0;
	std::vector<torrent_status> temp;
 	ses->get_torrent_status(&temp, &yes, 0);
	for (std::vector<torrent_status>::iterator i = temp.begin();
		i != temp.end(); ++i)
	{
		torrent_status& st = *i;
		if (!st.handle.is_valid())
		{
			//printf("  skipping, invalid handle\n");
			continue;
		}
		if (!st.has_metadata)
		{
			//printf("  skipping %s, no metadata\n", st.handle.name().c_str());
			continue;
		}
		if (!st.need_save_resume)
		{
			//printf("  skipping %s, resume file up-to-date\n", st.handle.name().c_str());
			continue;
		}

		// save_resume_data will generate an alert when it's done
		st.handle.save_resume_data();
		++num_outstanding_resume_data;
		printf("\r%d  ", num_outstanding_resume_data);
	}
	fprintf(stderr,"waiting for resume data [%d]\n", num_outstanding_resume_data);

	while (num_outstanding_resume_data > 0)
	{
		alert const* a = ses->wait_for_alert(seconds(10));
		if (a == 0) continue;

		std::deque<alert*> alerts;
		ses->pop_alerts(&alerts);
		std::string now = time_now_string();
		for (std::deque<alert*>::iterator i = alerts.begin()
			, end(alerts.end()); i != end; ++i)
		{
			// make sure to delete each alert
			std::auto_ptr<alert> a(*i);

			torrent_paused_alert const* tp = alert_cast<torrent_paused_alert>(*i);
			if (tp)
			{
//				printf("\rleft: %d failed: %d pause: %d "					, num_outstanding_resume_data, num_failed, num_paused);
				continue;
			}

			if (alert_cast<save_resume_data_failed_alert>(*i))
			{
//				++num_failed;
				--num_outstanding_resume_data;
//				printf("\rleft: %d failed: %d pause: %d "					, num_outstanding_resume_data, num_failed, num_paused);
				continue;
			}

			save_resume_data_alert const* rd = alert_cast<save_resume_data_alert>(*i);
			if (!rd) continue;
			--num_outstanding_resume_data;
			//printf("\rleft: %d failed: %d pause: %d "				, num_outstanding_resume_data, num_failed, num_paused);

			if (!rd->resume_data) continue;

			torrent_handle h = rd->handle;
			std::vector<char> out;
			bencode(std::back_inserter(out), *rd->resume_data);
			save_file( combine_path("CT_DATA", to_hex(h.info_hash().to_string()) + ".resume"), out);
		}
	}
	qDebug() << "saving session state" ;
	
	{
		entry session_state;
		ses->save_state(session_state);

		std::vector<char> out;
		bencode(std::back_inserter(out), session_state);
		save_file("CT_DATA/actual.state", out);
	}
	QFile pathDataFile("CT_DATA/path.resume");
	if (pathDataFile.open(QFile::WriteOnly))
	{
		for (QMap<QString,QString>::iterator i=save_path_data.begin();i!=save_path_data.end();i++)
		{
			pathDataFile.write((i.key()+"|"+i.value()+"\n").toUtf8());
		}
		pathDataFile.close();
	}
	ses->abort();

}
int TorrentManager::save_file(std::string const& filename, std::vector<char>& v)
{
	using namespace libtorrent;

	file f;
	error_code ec;
	if (!f.open(filename, file::write_only, ec)) return -1;
	if (ec) return -1;
	file::iovec_t b = {&v[0], v.size()};
	size_type written = f.writev(0, &b, 1, ec);
	if (written != int(v.size())) return -3;
	if (ec) return -3;
	return 0;
}

TorrentManager::~TorrentManager()
{
	
	qDebug() << "TorrentManager: object distruction";
	
	onClose();
	QApplicationSettings::FreeInstance();
}

TorrentManager* TorrentManager::_instance=NULL;
int TorrentManager::_instanceCount=0;
TorrentManager* TorrentManager::getInstance()
{
	if (_instance==NULL)
		_instance = new TorrentManager();
	_instanceCount++;
	qDebug() << "TorrentManager: someone ascked an instance";
	qDebug() << "TorrentManager: this is " << _instanceCount << " instance";
	return _instance;
}
void TorrentManager::freeInstance()
{
	qDebug() << "TorrentManager: free " << _instanceCount << " instance";
	_instanceCount--;
	if (!_instanceCount)
	{
		_instance->~TorrentManager();
		_instance=NULL;
	}
}
opentorrent_info* TorrentManager::GetTorrentInfo(QString filename)
{
	error_code ec;
	torrent_info* ti=new torrent_info(filename.toUtf8().data(), ec);
	if (ec)
	{
		QMessageBox::warning(NULL,"Warning",QString::fromLocal8Bit(("�� ������� ������� ������� ����\n"+filename).toAscii().data()));
		return NULL;
	}

	opentorrent_info* info=new opentorrent_info;
	info->size=ti->total_size();
	info->name=QString::fromUtf8(ti->name().c_str());
	info->describtion = QString::fromUtf8(ti->comment().c_str());
	info->files = ti->files();
	QMap<QString,int> suffixesCount;
	QString base_suffix;
	int maxSuffix=0;

	for(libtorrent::file_storage::iterator i=info->files.begin();i!=info->files.end();i++)
	{
		QFileInfo curfile(QString::fromUtf8(info->files.file_path(*i).c_str()));
		if (curfile.suffix()=="mds")
		{
			base_suffix="mdf";
			break;
		}
		if (curfile.suffix()=="mdf")
		{
			base_suffix="mdf";
			break;
		}
		if (curfile.suffix()=="m2ts")
		{
			base_suffix="m2ts";
			break;
		}
		if (!suffixesCount.contains(curfile.suffix()))
		{
			suffixesCount.insert(curfile.suffix(),1);
			if (suffixesCount[curfile.suffix()] > maxSuffix)
			{
				maxSuffix=suffixesCount[curfile.suffix()];
				base_suffix=curfile.suffix();
			}
		}
		else
		{

			suffixesCount[curfile.suffix()]++;
			if (suffixesCount[curfile.suffix()] > maxSuffix)
			{
				maxSuffix=suffixesCount[curfile.suffix()];
				base_suffix=curfile.suffix();
			}
		}
	}
	info->base_suffix=base_suffix;
	return info;
}
void TorrentManager::RemoveTorrent(torrent_handle h,bool delFiles)
{
	QFile::remove(combine_path("CT_DATA",h.get_torrent_info().name()+".torrent").c_str());
	QFile::remove(combine_path("CT_DATA",to_hex(h.info_hash().to_string())+".resume").c_str());
	save_path_data.remove(to_hex(h.info_hash().to_string()).c_str());
	ses->remove_torrent(h);
	/*if (delFiles)
	{
		QString path=(h.save_path()+h.name()).c_str();
		QFileInfo info(path);
		if (info.isDir())
		{
			qDebug() << path << "is dir";
			StaticHelpers::dellDir(path);
		}
		else
		{
			qDebug() << path << "is file";
			QFile::remove(path);
		}
	}*/
}
QString TorrentManager::GetSessionDownloadSpeed()
{
	try
	{
		return StaticHelpers::toKbMbGb(ses->status().download_rate)+"/s";
	}
	catch (...)
	{
		return "";
	}
	
}
QString TorrentManager::GetSessionUploadSpeed()
{
	try
	{
		return StaticHelpers::toKbMbGb(ses->status().upload_rate)+"/s";
	}
	catch (...)
	{
		return "";
	}
	
}
QString TorrentManager::GetSessionDownloaded()
{
	try
	{
		return StaticHelpers::toKbMbGb(ses->status().total_download);
	}
	catch (...)
	{
		return "";
	}
	
}
QString TorrentManager::GetSessionUploaded()
{
	
	try
	{
		return StaticHelpers::toKbMbGb(ses->status().total_upload);
	}
	catch (...)
	{
		return "";
	}

}