/*
 * Stellarium
 * Copyright (C) 2022 Alexander Wolf
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */
 
#include <QStringList>

namespace StelContributors {

    static const QStringList contributorsList =
    {
        "Fabien Chereau", "Georg Zotti", "Alexander V. Wolf", "Ruslan Kabatsayev", "Matthew Gates", 
        "Bogdan Marinov", "Rob Spearman", "Marcos Cardinot", "Guillaume Chereau", "Florian Schaukowitsch", 
        "Ferdinand Majerech", "Johannes Gajdosik", "Timothy Reaves", "Susanne M Hoffmann", "Tony Furr", 
        "Sun Shuwei", "Eleni Maria Stea", "Andrei Borza", "Teresa Huertas", "Diego Marcos", 
        "Johan Meuris", "Worachate Boonplod", "J.L.Canales", "Andras Mohari", "Henry Leung", 
        "Ivan Marti-Vidal", "Nicolas Martignoni", "Michael Storm", "Petr Kubánek", "Holger Nießner", 
        "Yuri Chornoivan", "Simon Parzer", "Qam1", "misibacsi", "Barry Gerdes", 
        "Alexey Sokolov", "Wang Siliang", "Anton Samoylov", "Khalid AlAjaji", "Adriano Steffler", 
        "Minmin Gong", "Alexander Duytschaever", "Nick Fedoseev", "Sibi Antony", "Tanmoy Saha", 
        "Pluton Helleformer", "Alex Gamper", "Atque", "Vishvas Vasuki विश्वासः", "Vladislav Bataron", 
        "Alessandro Siniscalchi", "Elaina", "Kutaibaa Akraa", "Luca-Philipp Grumbach", "Peter Mousley", 
        "Andy Kirkham", "Jörg Müller", "Peter Vasey", "Rumen Bogdanovski", "Dan Smale", 
        "Hernan Martinez", "henrysky", "FreewareTips", "RVS", "Wolfgang Laun", 
        "luzpaz", "uwes-ufo", "Allan Johnson", "Colin Gaudion", "Emmanuel", 
        "Greg Alexander", "Michal Sojka", "Peter Walser", "William Formyduval", "Giuseppe Putzolu", 
        "Fòram na Gàidhlig", "Paolo Cancedda (Pac)", "Pawel Stolowski", "Ralph Schäfermeier", "Teemu Nätkinniemi", 
        "Volker Hören", "ChrUnger", "Chris Xiao", "Hleb Valoshka", "Iceflower", 
        "Luz Paz", "Mircea Lite", "Paul Krizak", "Ray", "Sergej Krivonos", 
        "Shantanu Agarwal", "Nick Kanel", "colossatr0n", "Martin Bernardi", "Miroslav Broz", 
        "ysjbserver", "Alexandros Kosiaris", "Alexey Dokuchaev", "Aspere", "Clement Sommelet", 
        "Daniel", "David Baucum", "Diego Sandoval", "EuklidAlexandria", "Felix Z", 
        "Freeman Li", "Hans Lambermont", "Jocelyn GIROD", "Jonas Persson", "Josh Meyers", 
        "Kirill Snezhko", "Konrad Rybka", "Maciej Serylak", "Max Digruber", "Mike Boyle", 
        "Mike Garrahan", "Norman Rasmussen", "Peter Neubauer", "Robert S. Fuller", "Sergey", 
        "Steven Bellavia", "Victor Reijs", "bv6679", "Daniel Adastra", "sebagr", 
        "ssimard2504", "tofilwiktor", "Adam Majer", "Alec Clews", "Alexander Belopolsky", 
        "Alexander Miller", "Andrew Jeddeloh", "Angelo Fraietta", "Annette S. Lee", "Antoine Jacoutot", 
        "Arjen de Korte", "Bernd K", "Björn Höfling", "Brian Kloppenborg", "Cassy", 
        "Chi Huynh", "Clepalitto", "Cosimo Cecchi", "Dan Joplin", "Daniel De Mickey", 
        "Daniel Michalik", "Danny Milosavljevic", "Dark Dragon", "Dempsey-p", "Dhia", 
        "Dominik Maximilián Ramík", "Fabian Hofer", "François Scholder", "Froenchenko Leonid", "Gion Kunz", 
        "GitHaarek", "Guillaume Communie", "Gábor Péterffy", "Hector Quemada", "Holger", 
        "Imad Saddik", "JAY RESPLER", "JMejuto", "Jack Schmidt", "Jean-Philippe Lambert", 
        "Katrin Leinweber", "Kenan Dervišević", "Koya_Azk", "Louis Strous", "M.S. Adityan", 
        "MaraJadeLives", "Martin Bernardi (Laptop)", "Matt Hughes", "Matthias Drochner", "Matwey V. Kornilov", 
        "Michael Dickens", "Michael Taylor", "Mykyta Sytyi", "Nidroide", "Nir Lichtman", 
        "Oleg Ginzburg", "Oscar Roig Felius", "Paolo Stivanin", "ParkSangJun", "Patrick", 
        "Paul Evans", "Pavel Klimenko", "Peter", "Peter Hickey", "Pino Toscano", 
        "Roland Bosa", "Sam Lanning", "Sebastian Jennen", "SilverAstro", "Song Li", 
        "Sripath Roy Koganti", "Sveinn í Felli", "Sylvain Simard", "ThibaudGS", "Thilo", 
        "Thomas1664", "Tig la Pomme", "Tomasz Buchert", "TotalCaesar659", "Tuomas Teipainen", 
        "Tēvita O. Kaʻili", "Vancho Stojkoski", "Vicente Reyes", "Vincent Gerlach", "Wonkyo Choe", 
        "Yaakov Selkowitz", "Youssif Ghantous Filho", "Yury Solomonov", "adalava", "afontenot", 
        "bkuhls", "chithihuynh", "dusan-prascevic", "Marc Espie", "leonardcj", 
        "luz paz", "mike406", "pkrawczun", "Pavel Klimenko", "rikardfalkeborn", 
        "riodoro1", "Ross Mitchell", "Edgar Scholz", "zhu1995zpb", "Łukasz 'sil2100' Zemczak"
    };
    
}

