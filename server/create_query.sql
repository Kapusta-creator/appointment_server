DROP table if EXISTS cards;
CREATE table cards (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    first_name TEXT NOT NULL,
    second_name TEXT NOT NULL,
    last_name TEXT NOT NULL,
    passport TEXT NOT NULL UNIQUE,
    phone_number TEXT UNIQUE,
    birthdate TEXT,
    verified INTEGER NOT NULL
);
CREATE INDEX card_id ON cards(id);

Drop table IF EXISTS types;
CREATE table types(
    id UNSIGNED INTEGER PRIMARY KEY NOT NULL,
    name CHAR NOT NULL
);

Drop table IF EXISTS branches;
CREATE table branches(
    id INTEGER PRIMARY KEY NOT NULL,
    name CHAR NOT NULL
);

Drop table IF EXISTS schedules;
create table schedules(
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    weekday INTEGER NOT NULL,
    begin_time text not null,
    end_time text not null,
    doc_id INTEGER NOT NULL,
    FOREIGN KEY(doc_id) REFERENCES doctors(id) ON UPDATE CASCADE ON DELETE CASCADE
);
CREATE INDEX schedule_id ON schedules(id);

Drop table IF EXISTS doctors;
CREATE table doctors(
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    first_name CHAR NOT NULL,
    second_name CHAR NOT NULL,
    max_time INTEGER,
    phone CHAR,
    type_id INTEGER,
    branch_id INTEGER,
    FOREIGN KEY(type_id) REFERENCES types(id) ON UPDATE CASCADE,
    FOREIGN KEY(branch_id) REFERENCES branches(id) ON UPDATE CASCADE
);
create INDEX doctor_id ON doctors(id);

Drop table IF EXISTS talons;
CREATE table talons(
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    begin_time TEXT NOT NULL,
    end_time TEXT NOT NULL,
    status_id INT NOT NULL,
    date TEXT NOT NULL,
    doc_id INTEGER NOT NULL,
    patient_id INTEGER NOT NULL,
    UNIQUE (begin_time, end_time, date, doc_id) ON CONFLICT REPLACE,
    FOREIGN KEY(doc_id) REFERENCES doctors(id) ON UPDATE CASCADE ON DELETE CASCADE,
    FOREIGN KEY(patient_id) REFERENCES cards(id) ON UPDATE CASCADE ON DELETE CASCADE
);
create INDEX talon_id on talons(id);
create INDEX doc_id on talons(doc_id);
create INDEX patient_id on talons(patient_id);

