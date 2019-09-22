CREATE USER IF NOT EXISTS 'admin'@'%' IDENTIFIED BY 'admin';
CREATE USER IF NOT EXISTS 'manager'@'%' IDENTIFIED BY 'manager';
CREATE USER IF NOT EXISTS 'spazi'@'%' IDENTIFIED BY 'spazi';
CREATE USER IF NOT EXISTS 'dipendente'@'%' IDENTIFIED BY 'dipendente';

CREATE SCHEMA Azienda;
USE Azienda;

GRANT ALL PRIVILEGES ON Azienda.* TO 'admin'@'%';

CREATE TABLE Mansione(
       Nome char(64) primary key
);


CREATE TABLE Dipendente(
       CodiceFiscale char(16) Primary key,
       Nome varchar(64) not null,
       Cognome varchar(64) not null,
       Email varchar(64) unique not null,
       DataDiNascita Date not null,
       LuogoDiNascita varchar(64) not null,
       Residenza varchar(64) not null,
       Mansione char(64) not null,
       Trasferimento bool not null default FALSE,
       foreign key (Mansione)
       	       references Mansione(Nome)
);

CREATE TABLE Edificio(
       Sede varchar(64) primary key
);

CREATE TABLE Ufficio(
       Codice int primary key AUTO_INCREMENT,
       Nome varchar(64) not null,
       Piano varchar(64) not null,
       Edificio varchar(64) not null,
       Mansione char(64) not null,
       foreign key (Mansione)
       	       references Mansione(Nome)
);

CREATE TABLE Postazione(
       Telefono char(32) primary key,
       Email varchar(64) not null unique,
       Ufficio int not null,
       foreign key (Ufficio)
       	       references Ufficio(Codice)
);

CREATE TABLE Trasferimento(
       Codice int primary key AUTO_INCREMENT,
       Dipendente char(16) not null,
       DataDiTrasferimento Date not null,
       Postazione char(32) not null,
       DataFineTrasferimento Date DEFAULT NULL,
       foreign key (Dipendente)
       	       references Dipendente(CodiceFiscale),
       foreign key (Postazione)
       	       references Postazione(Telefono)
);

CREATE INDEX NomeCognomeD on Dipendente(Nome, Cognome);

DELIMITER $$
CREATE TRIGGER VincoloTraferimento
BEFORE INSERT ON Trasferimento
FOR EACH ROW
BEGIN
	DECLARE D_MANSIONE varchar(64);
	DECLARE U_MANSIONE varchar(64);
	DECLARE NO_P_LAST_3_YEAR INT;
	DECLARE NO_ACTIVE_TRANSFERIMENT INT;
	-- Se il dipendente e l'ufficio inserito non hanno la stessa mansione
	-- annulla l'insert mettendo NEW.Dipendente a NULL andando
	-- a violare il vincolo not null della tabella.
	SELECT Mansione INTO D_MANSIONE
	FROM Dipendente AS D
	WHERE D.CodiceFiscale = NEW.Dipendente;
	
	SELECT Mansione INTO U_MANSIONE
	FROM Ufficio AS U JOIN Postazione as P
	     ON U.Codice = P.Ufficio
	WHERE P.Telefono = NEW.Postazione;
	
	IF D_MANSIONE <> U_MANSIONE THEN
	   SET NEW.Dipendente = NULL;
	END IF;
	-- Conta il numero di trasferimenti negli ultimi 3 anni
	-- che hanno lo stesso dipendente e lo stessa postazione
	-- se sono un numero diverso da 0, annulla l'inserimento.
	SELECT COUNT(*) INTO NO_P_LAST_3_YEAR
	FROM Trasferimento as T
	WHERE T.DataDiTrasferimento BETWEEN
	      			    CURRENT_DATE - INTERVAL 3 YEAR
	      		   	    AND
	      			    CURRENT_DATE
	      AND
	      T.Dipendente = NEW.Dipendente
	      AND
	      T.Postazione = NEW.Postazione;

	IF NO_P_LAST_3_YEAR <> 0 THEN
	   SET NEW.Dipendente = NULL;
	END IF;
	-- Se il dipendente ha una trasferimento in corso annulla il nuovo
	-- trasferimento, serve a fare in modo che un dipendente possa
	-- avere un solo dipendente attivo alla volta.

	SELECT COUNT(*) INTO NO_ACTIVE_TRANSFERIMENT
	FROM Trasferimento
	WHERE Dipendente = NEW.Dipendente AND DataFineTrasferimento = NULL;

	IF NO_ACTIVE_TRANSFERIMENT <> 0 THEN
	   SET NEW.Dipendente = NULL;
	END IF;

	-- Se trasferisci un dipendente cambia il suo stato di trasferimento.
	UPDATE Dipendente
	       SET Trasferimento = FALSE
	       WHERE CodiceFiscale = NEW.Dipendente;
	       
END;
$$

CREATE TRIGGER VincoloCambioMansione
BEFORE UPDATE ON Dipendente
FOR EACH ROW
BEGIN
	-- Se viene cambiata la mansione di un dipendente
	-- aggiorna il suo stato di trasferimento.
	IF NEW.Mansione <> OLD.Mansione THEN
	   SET NEW.Trasferimento = TRUE;
	END IF;
END;
$$

CREATE PROCEDURE RicercaDipendente(IN nome varchar(64), cognome varchar(64))
BEGIN
	-- Ritorna i recapiti di un dipendente dato almeno uno tra nome e cognome
	-- di un dipendente.
	SELECT D.CodiceFiscale, D.Nome, D.Cognome, D.Email, P.Telefono, P.Email, U.Nome, U.Edificio
	FROM Dipendente as D LEFT JOIN Trasferimento AS T ON D.CodiceFiscale = T.Dipendente AND T.DataFineTrasferimento IS NULL
	     JOIN
	     Postazione AS P JOIN Ufficio AS U ON P.Ufficio = U.Codice
	     ON T.Postazione = P.Telefono
	WHERE (nome IS NULL OR D.Nome = nome) AND (cognome is NULL OR D.Cognome = cognome)
	ORDER BY D.Cognome, D.Nome;
END;
$$

CREATE PROCEDURE RicercaUfficio(IN telefono char(32))
BEGIN
	-- Dato il telefono che identifica una postazione
	-- ritorna l'uffico a cui appartiene.
	SELECT U.Codice, U.Nome, U.Piano, U.Edificio
	FROM Postazione AS P JOIN Ufficio AS U
	     ON P.Ufficio = U.Codice
	WHERE P.Telefono = telefono;
END;
$$

CREATE PROCEDURE ListaDipendentiUfficio(IN ufficio INT)
BEGIN
	-- Dato un ufficio ritorna tutti i dipendenti che ne
	-- fanno parte, unito allo stato di trasferimento.
	SELECT D.CodiceFiscale, D.Nome, D.Cognome, D.Trasferimento
	FROM Postazione AS P JOIN Ufficio AS U ON P.Ufficio = U.Codice
	     JOIN
	     Dipendente AS D JOIN Trasferimento AS T
	     		ON D.CodiceFiscale = T.Dipendente AND T.DataFineTrasferimento IS NULL
	     ON T.Postazione = P.Telefono
	WHERE U.Codice = ufficio;
	     
END;
$$


CREATE PROCEDURE ListaDipendentiDaTrasferire()
BEGIN
	-- Ritorna tutti i dipendenti da trasferire.
	SELECT CodiceFiscale, Nome, Cognome, Mansione
	FROM Dipendente
	WHERE Trasferimento = TRUE;
END;
$$


CREATE PROCEDURE TrasferisciDipendente(IN dipendente char(16), IN postazione char(32))
BEGIN
	-- Trasferisce un dipendente in una nuova postazione libera.
	DECLARE curr_date Date;
	DECLARE last_transfer INT;
	-- Dichiara cosa la procedura deve fare in caso di
	-- errori durante una transazione.
	DECLARE exit handler for sqlexception
	  BEGIN
	     -- ERROR
	     ROLLBACK;
	     -- propaga l'errore al caller.
	     RESIGNAL;
	  END;
 
	DECLARE exit handler for sqlwarning
	 BEGIN
	     -- WARNING
	     ROLLBACK;
	     -- propaga l'errore al caller.	     
	     RESIGNAL;
	END;
	-- Il trasferimento deve essere una procedura atomica, ma deve modificare
	-- i valori su tabelle differenti.
	START TRANSACTION;
	-- data di trasferimento
	SET curr_date = CURRENT_DATE;
	-- Trova l'ultimo trasferimento attivo.
	SELECT Codice INTO last_transfer
	FROM Trasferimento AS T
	WHERE T.Dipendente = dipendente AND T.DataFineTrasferimento IS NULL;
	-- Se vi era un trasferimento attivo, lo chiudiamo.
	UPDATE Trasferimento
	       SET DataFineTrasferimento = curr_date
	       WHERE Codice = last_transfer AND DataFineTrasferimento IS NULL;
	-- Inserisci i dati del nuovo trasferimento.
	INSERT INTO Trasferimento(Dipendente, DataDiTrasferimento, Postazione, DataFineTrasferimento)
	       VALUES (dipendente, curr_date, postazione, NULL);
	       
	COMMIT;
END;
$$


CREATE PROCEDURE TrasferisciDipendenteSWAP(IN dipendente_1 char(16), IN dipendente_2 char(16))
BEGIN
	-- Scambia la postazione di due dipendenti aventi
	-- la stessa mansione e entrambi da trasferire.
	DECLARE postazione_1 char(32);
	DECLARE postazione_2 char(32);
	DECLARE curr_date Date;
	-- Dichiara come la procedura deve comportarsi durante la transazione
	-- in caso di errore.
	DECLARE exit handler for sqlexception
	 BEGIN
	    -- ERROR
	    ROLLBACK;
	    RESIGNAL;
	 END;
 
	DECLARE exit handler for sqlwarning
	 BEGIN
	     -- WARNING
	     ROLLBACK;
	     RESIGNAL;
	END;


	START TRANSACTION;
	-- data di trasferimento.
	SET curr_date = CURRENT_DATE;
	-- trova la postazione corrente dei due dipendenti.
	SELECT Postazione into postazione_1
	FROM Trasferimento AS T
	WHERE T.DataFineTrasferimento IS NULL AND T.Dipendente = dipendente_1;
	
	SELECT Postazione into postazione_2
	FROM Trasferimento AS T
	WHERE T.DataFineTrasferimento IS NULL AND T.Dipendente = dipendente_2;
	-- chiudi lo stato del precedente trasferimento.
	UPDATE Trasferimento
	       SET DataFineTrasferimento = curr_date
	       WHERE Dipendente = dipendente_1 OR Dipendente = dipendente_2;
	-- inserisci le nuove postazioni, se una delle due è null l'inserimento
	-- fallisce.
	INSERT INTO Trasferimento(Dipendente, DataDiTrasferimento, Postazione)
	       VALUES (dipendente_1, curr_date, postazione_2),
		      (dipendente_2, curr_date, postazione_1);
	COMMIT;
END;
$$


CREATE PROCEDURE ModificaMansione(IN dipendente char(16), IN nuova_mansione char(64))
BEGIN
	-- modifica la mansione di un dipendente.
	UPDATE Dipendente
	       SET Mansione = nuova_mansione
	       WHERE CodiceFiscale = dipendente;
END;
$$

CREATE PROCEDURE StoricoDipendente(IN dipendente char(16))
BEGIN
	-- Ritorna lo storico dei trasferimenti di un dipendente.
	SELECT T.DataDiTrasferimento, T.DataFineTrasferimento, P.Telefono, U.Nome, U.Edificio, U.Mansione
	FROM Trasferimento AS T JOIN Postazione AS P ON T.Postazione = P.Telefono
	     JOIN Ufficio AS U ON P.Ufficio = U.Codice
	WHERE T.Dipendente = dipendente;
END;
$$


CREATE PROCEDURE ListaUffici()
BEGIN
	-- Ritorna tutti gli uffici presenti nell'azienda.
	SELECT Nome, Piano, Edificio, Mansione
	FROM Ufficio;
END;
$$


CREATE PROCEDURE ListaPostazioni()
BEGIN
	-- ritorna tutte le postazioni presenti nell'azienda.
	SELECT Telefono, Email, Nome, Piano, Edificio, Mansione
	FROM Ufficio AS U JOIN Postazione AS P
	     ON P.Ufficio = U.Codice;
END;
$$

-- Postazione
-- Stessa mansione del dipendente
-- Il dipendente non deve esserci stato negli ultimi 3 anni
-- La postazione è libera o il dipendente che la occupa è da trasferire.
CREATE PROCEDURE ListaPostazioniDisponibili(IN dipendente char(16))
BEGIN
	-- Dato un dipendete ritorna tutte le postazioni in cui può
	-- essere trasferito.
	-- Le postazioni devono avere le seguente proprietà:
	-- 1. Devono appartenere a uffici aventi la stessa
	--    mansione del dipendente.
	-- 2. Il dipendente non deve esserci stato assegnato negli
	--    ultimi 3 anni.
	-- 3. La postazione deve essere libera, o se occupata il dipendente
	--    che la detiene deve essere anche lui da trasferire.
	SELECT P.Telefono, U.Nome, U.Edificio, D.CodiceFiscale
	FROM Dipendente AS D JOIN Trasferimento AS T
	     		ON D.CodiceFiscale = T.Dipendente
	     RIGHT JOIN
	     Postazione AS P JOIN Ufficio AS U
	     		ON P.Ufficio = U.Codice
		ON T.Postazione = P.Telefono
 	WHERE U.Mansione = (SELECT Mansione
	      		    FROM Dipendente
			    WHERE CodiceFiscale = dipendente)
	      AND
	      P.Telefono NOT IN (SELECT Postazione
	      		     	  FROM Trasferimento AS T
				  WHERE T.Dipendente = dipendente
				  	AND
					T.DataDiTrasferimento BETWEEN
					CURRENT_DATE - INTERVAL 3 YEAR
					AND
					CURRENT_DATE)
	      AND
	      P.Telefono NOT IN (SELECT Postazione
	      		         FROM Dipendente AS D JOIN Trasferimento AS T
				      ON D.CodiceFiscale = T.Dipendente
				 WHERE T.Dipendente <> dipendente
				       AND
				       D.Trasferimento = FALSE
				       AND
				       T.DataFineTrasferimento IS NULL);
END;
$$

-- evento
CREATE EVENT TurnoTrasferimento
ON SCHEDULE EVERY '1' DAY
STARTS CURRENT_TIMESTAMP
DO
BEGIN
	-- Ogni giorno controlla se un dipendente deve essere trasferito.
	-- Il periodi di trasferimento è stato inserito come 6 mesi.
	UPDATE Dipendente AS D
		JOIN Trasferimento AS T
	       	    ON T.DataFineTrasferimento IS NULL AND T.Dipendente = D.CodiceFiscale
	       SET Trasferimento = TRUE
	       WHERE T.DataDiTrasferimento NOT
	       	     			   BETWEEN
					   CURRENT_DATE - INTERVAL 6 MONTH
	       	     			   AND
					   CURRENT_DATE;
END;
$$
DELIMITER ;

-- Permission 
GRANT EXECUTE ON PROCEDURE Azienda.RicercaDipendente TO 'dipendente'@'%';
GRANT EXECUTE ON PROCEDURE Azienda.RicercaDipendente TO 'spazi'@'%';
GRANT EXECUTE ON PROCEDURE Azienda.RicercaDipendente TO 'manager'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.RicercaUfficio TO 'dipendente'@'%';
GRANT EXECUTE ON PROCEDURE Azienda.RicercaUfficio TO 'spazi'@'%';
GRANT EXECUTE ON PROCEDURE Azienda.RicercaUfficio TO 'manager'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.ListaDipendentiUfficio TO 'dipendente'@'%';
GRANT EXECUTE ON PROCEDURE Azienda.ListaDipendentiUfficio TO 'spazi'@'%';
GRANT EXECUTE ON PROCEDURE Azienda.ListaDipendentiUfficio TO 'manager'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.ListaDipendentiDaTrasferire TO 'spazi'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.TrasferisciDipendente TO 'manager'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.TrasferisciDipendenteSWAP TO 'manager'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.ModificaMansione TO 'manager'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.StoricoDipendente TO 'manager'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.ListaUffici TO 'spazi'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.ListaPostazioni TO 'spazi'@'%';

GRANT EXECUTE ON PROCEDURE Azienda.ListaPostazioniDisponibili TO 'spazi'@'%';

-- DATA INIT --

INSERT INTO Mansione(Nome)
       VALUES ('Cattura Pikachu'),
	      ('Generale Rocket'),
	      ('Capo Team Rocket');

INSERT INTO Dipendente(CodiceFiscale, Nome, Cognome, DataDiNascita, LuogoDiNascita, Residenza, Email, Mansione)
       VALUES ('0001', 'Giovanni', 'Rocket', '1996-02-27', 'Smeraldopoli', 'Sede Team Rocket', 'Giovanni@Rocket.com', 'Capo Team Rocket'),
       	      ('0002', 'Archie', 'GeneraleRocket', '1999-11-21', 'Quintisola', 'Magazzino Rocket', 'Archer@Rocket.com', 'Generale Rocket'),
	      ('0003', 'Sabrina', 'Psico', '1996-02-27', 'Zafferanopoli', 'Palestra Zaffernapoli', 'Sabrina@Rocket.com', 'Generale Rocket'),
	      ('0004', 'Koga', 'Veleno', '1996-02-27', 'Fucsiapoli', 'Palestra Fucsiapoli', 'Koga@Rocket.com', 'Generale Rocket'),
	      ('0005', 'Surge', 'Fulmine', '1996-02-27', 'Aranciopoli', 'Palestra Aranciopoli', 'Surge@Rocket.com', 'Generale Rocket'),
	      ('0006', 'Jessie', 'Meowth','1996-02-27', 'Smeraldopoli', 'Sede Team Rocket', 'Jessie@Rocket.com', 'Cattura Pikachu'),
	      ('0007', 'James', 'Meowth','1996-02-27', 'Smeraldopoli', 'Sede Team Rocket', 'James@Rocket.com', 'Cattura Pikachu');

INSERT INTO Edificio(Sede) VALUES ('Sede Team Rocket');

INSERT INTO Ufficio (Nome, Piano, Edificio, Mansione)
       VALUES ('U0', 'P0', 'Sede Team Rocket', 'Cattura Pikachu'),
       	      ('U1', 'P0', 'Sede Team Rocket', 'Generale Rocket'),
	      ('U2', 'P1', 'Sede Team Rocket', 'Capo Team Rocket');
	      

INSERT INTO Postazione (Telefono, Email, Ufficio)
       VALUES ('0001', 'Pika@Rocket.com', 1),
       	      ('0002', 'Chu@Rocket.com', 1),
	      ('0003', 'Generale_1@Rocket.com', 2),
       	      ('0004', 'Generale_2@Rocket.com', 2),
	      ('0005', 'Generale_3@Rocket.com', 2),
	      ('0006', 'Generale_4@Rocket.com', 2),
	      ('0007', 'CapoTeamRocket@Rocket.com', 3),
	      ('0008', 'ChuChu@Rocket.com', 1),
	      ('0009', 'PikaPika@Rocket.com', 1),
	      ('0010', 'PikaChu@Rocket.com', 2),
	      ('0011', 'ChuPika@Rocket.com', 2),
	      ('0012', 'Pichu@Rocket.com', 3);

INSERT INTO Trasferimento (Dipendente, DataDiTrasferimento, Postazione)
       VALUES ('0001', CURRENT_DATE, '0007'),
       	      ('0002', CURRENT_DATE, '0006'),
	      ('0003', CURRENT_DATE, '0005'),
	      ('0004', CURRENT_DATE, '0004'),
	      ('0005', CURRENT_DATE, '0003'),
	      ('0006', CURRENT_DATE, '0002'),
	      ('0007', CURRENT_DATE, '0001');
