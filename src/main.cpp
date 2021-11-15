// NetPolis

// Pas d'historique coté serveur:
// Ses missions sont d'enregistrer les evenements des clients et de les associer a un tick
// Ces evenements n'impliquent pas une valeur absolue: c'est le serveur qui l'établie.
// Peut etre que le client pourrait transmettre a titre indicatif une valeur absolue... mais pas sur.
// Si il le fait, alors il verifiera la validité de l'action (y compris dans le temps)
// - - - - -
// Il transmets les evenements a tous le monde en les associant au tick.
// Il les transmet aussi a l'emetteur, de sorte a ce qu'il puisse verifier que tout va bien

// Coté client: historique des actions, de sorte a pouvoir reconcilier les actions issues
// des autres joueurs avec les actions locales

enum		Direction
  {
   LEFT,
   RIGHT,
   UP,
   DOWN
  }

struct		Action
{
  Direction	direction;
  bool		pressed;
};

struct		Player
{
  t_bunny_position pos;
  bool		pressed[4];
  std::map<int, bool> see_player;
}

struct		PlayerMoved
{
  int		tick;
  int		fd;
  t_bunny_position pos;
  bool		pressed[4];
};

struct		PlayerOut
{
  int		tick;
  int		fd;
};

int		server(const char	*port)
{
  t_bunny_server *srv;
  char		*end;
  int		prt;

  prt = strtol(port, &end, 0);
  if (end == port || prt < 1024)
    {
      puts("Invalid port");
      return (EXIT_FAILURE);
    }
  if ((srv = bunny_new_server(prt)) == NULL)
    {
      puts("Cannot open server");
      return (EXIT_FAILURE);
    }
  std::map<int, Player> players;
  int tick = 0;

  while (1)
    {
      t_bunny_time start = bunny_get_time();

      do
	{
	  const t_bunny_communication *com = bunny_server_poll(srv, 0);

	  if (com->comtype == BCT_ERROR)
	    {
	      puts("A server error happened");
	      return (EXIT_FAILURE);
	    }
	  if (com->comtype == BCT_NETCONNECTED)
	    {
	      players[com->connected.fd] = t_bunny_position{0, 0};
	      bunny_server_write(srv, &com->connected.fd, sizeof(com->connected.fd), com->connected.fd);
	    }
	  else if (com->comtype == BCT_NETDISCONNECTED)
	    players.erase(com->disconnected.fd);
	  else if (com->comtype == BCT_MESSAGE)
	    {
	      t_bunny_message &msg = com->message;
	      const Action *act = msg.buffer;
	      auto &plr = players[msg.fd];

	      plr.pressed[act->direction] = act->pressed;

	      // Broadcast.
	      PlayerMoved pm{tick, msg.fd, plr.pos, plr.pressed};

	      for (auto it = players.begin(); it != players.end(); ++it)
		if (it->first != msg.fd)
		  {
		    // Les joueurs se voient mutuellement
		    if (sqrt(pow(it->pos.x - plr.pos.x, 2) + pow(it->pos.y - plr.pos.y, 2)) < 255)
		      {
			bunny_server_write(srv, &pm, sizeof(pm), it->first);
			plr.see_player[it->first] = true;
		      }
		  }
	    }
	}
      while (bunny_server_packet_ready(srv));

      //////////////////////////
      //// BOUCLE PRINCIPALE ///
      //////////////////////////

      // On bouge les joueurs en accord avec la vitesse du jeu
      for (auto it = players.begin(); it != players.end(); ++it)
	{
	  Player &p = it->second;

	  if (p.pressed[LEFT])
	    p.pos.x -= 1;
	  if (p.pressed[RIGHT])
	    p.pos.x += 1;
	  if (p.pressed[UP])
	    p.pos.y -= 1;
	  if (p.pressed[DOWN])
	    p.pos.y += 1;
	}

      // Pour signaler aux clients que certains joueurs ne se voient plus.
      for (auto ita = players.begin(); ita != players.end(); ++it)
	{
	  auto &plra = ita->second;
	  PlayerOut po{tick, ita->first};

	  for (auto itb = players.begin(); ita->first != itb->first && itb != players.end(); ++it)
	    {
	      auto &plrb = itb->second;

	      if (sqrt(pow(plra.pos.x - plrb.pos.x, 2) + pow(plra.pos.y - plrb.pos.y, 2)) >= 255)
		if (plra.see_player[itb->first])
		  {
		    bunny_server_write(srv, &po, sizeof(po), ita->first);
		    plra.see_player[itb->first] = false;
		  }
	    }
	}

      tick += 1;
      // De temps en temps, on envoi une image clef
      if (tick % 20 == 0)
	{
	  //
	}

      t_bunny_time after = bunny_get_time();

      if (after - start < 10 * 1e6)
	bunny_usleep((10 * 1e6 - (after - start)) / 1e3); // On dors pour faire un cycle de 10ms
    }

  bunny_delete_server(srv);
}

int		client(const char	*ip,
		       const char	*port)
{
  char		*end;
  int		prt;

  prt = strtol(port, &end, 0);
  if (end == port || prt < 1024)
    {
      puts("Invalid port");
      return (EXIT_FAILURE);
    }
  
}

int		main(int		argc,
		     char		**argv)
{
  if (argc == 2)
    return (server(argv[1]));
  if (argc == 3)
    return (client(argv[1], argv[2]));
  puts("./netpolis port | ./netpolis ip port")
  return (EXIT_FAILURE);
}
