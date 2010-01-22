-- Triggers for foreign key mapping:
--
--     observations(imager_id) REFERENCES imagers(imager_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey4_insert_referencing BEFORE INSERT ON "observations" WHEN new."imager_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "imagers" WHERE new."imager_id" == "imager_id") BEGIN SELECT RAISE(ABORT, 'genfkey4_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey4_update_referencing BEFORE UPDATE OF imager_id ON "observations" WHEN new."imager_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "imagers" WHERE new."imager_id" == "imager_id") BEGIN SELECT RAISE(ABORT, 'genfkey4_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey4_delete_referenced BEFORE DELETE ON "imagers" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."imager_id" == "imager_id") BEGIN SELECT RAISE(ABORT, 'genfkey4_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey4_update_referenced AFTER UPDATE OF imager_id ON "imagers" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."imager_id" == "imager_id") BEGIN SELECT RAISE(ABORT, 'genfkey4_update_referenced constraint failed'); END;
 
-- Triggers for foreign key mapping:
--
--     observations(filter_id) REFERENCES filters(filter_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey5_insert_referencing BEFORE INSERT ON "observations" WHEN new."filter_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "filters" WHERE new."filter_id" == "filter_id") BEGIN SELECT RAISE(ABORT, 'genfkey5_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey5_update_referencing BEFORE UPDATE OF filter_id ON "observations" WHEN new."filter_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "filters" WHERE new."filter_id" == "filter_id") BEGIN SELECT RAISE(ABORT, 'genfkey5_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey5_delete_referenced BEFORE DELETE ON "filters" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."filter_id" == "filter_id") BEGIN SELECT RAISE(ABORT, 'genfkey5_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey5_update_referenced AFTER UPDATE OF filter_id ON "filters" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."filter_id" == "filter_id") BEGIN SELECT RAISE(ABORT, 'genfkey5_update_referenced constraint failed'); END;
 
-- Triggers for foreign key mapping:
--
--     observations(barlow_id) REFERENCES barlows(barlow_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey6_insert_referencing BEFORE INSERT ON "observations" WHEN new."barlow_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "barlows" WHERE new."barlow_id" == "barlow_id") BEGIN SELECT RAISE(ABORT, 'genfkey6_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey6_update_referencing BEFORE UPDATE OF barlow_id ON "observations" WHEN new."barlow_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "barlows" WHERE new."barlow_id" == "barlow_id") BEGIN SELECT RAISE(ABORT, 'genfkey6_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey6_delete_referenced BEFORE DELETE ON "barlows" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."barlow_id" == "barlow_id") BEGIN SELECT RAISE(ABORT, 'genfkey6_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey6_update_referenced AFTER UPDATE OF barlow_id ON "barlows" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."barlow_id" == "barlow_id") BEGIN SELECT RAISE(ABORT, 'genfkey6_update_referenced constraint failed'); END;

-- Triggers for foreign key mapping:
--
--     observations(ocular_id) REFERENCES oculars(ocular_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey7_insert_referencing BEFORE INSERT ON "observations" WHEN new."ocular_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "oculars" WHERE new."ocular_id" == "ocular_id") BEGIN SELECT RAISE(ABORT, 'genfkey7_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey7_update_referencing BEFORE UPDATE OF ocular_id ON "observations" WHEN new."ocular_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "oculars" WHERE new."ocular_id" == "ocular_id") BEGIN SELECT RAISE(ABORT, 'genfkey7_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey7_delete_referenced BEFORE DELETE ON "oculars" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."ocular_id" == "ocular_id") BEGIN SELECT RAISE(ABORT, 'genfkey7_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey7_update_referenced AFTER UPDATE OF ocular_id ON "oculars" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."ocular_id" == "ocular_id") BEGIN SELECT RAISE(ABORT, 'genfkey7_update_referenced constraint failed'); END;
 
-- Triggers for foreign key mapping:
--
--     observations(optics_id) REFERENCES optics(optic_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey8_insert_referencing BEFORE INSERT ON "observations" WHEN new."optics_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "optics" WHERE new."optics_id" == "optic_id") BEGIN SELECT RAISE(ABORT, 'genfkey8_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey8_update_referencing BEFORE UPDATE OF optics_id ON "observations" WHEN new."optics_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "optics" WHERE new."optics_id" == "optic_id") BEGIN SELECT RAISE(ABORT, 'genfkey8_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey8_delete_referenced BEFORE DELETE ON "optics" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."optic_id" == "optics_id") BEGIN SELECT RAISE(ABORT, 'genfkey8_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey8_update_referenced AFTER UPDATE OF optic_id ON "optics" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."optic_id" == "optics_id") BEGIN SELECT RAISE(ABORT, 'genfkey8_update_referenced constraint failed'); END;
 
-- Triggers for foreign key mapping:
--
--     observations(target_id) REFERENCES targets(target_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey9_insert_referencing BEFORE INSERT ON "observations" WHEN new."target_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "targets" WHERE new."target_id" == "target_id") BEGIN SELECT RAISE(ABORT, 'genfkey9_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey9_update_referencing BEFORE UPDATE OF target_id ON "observations" WHEN new."target_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "targets" WHERE new."target_id" == "target_id") BEGIN SELECT RAISE(ABORT, 'genfkey9_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey9_delete_referenced BEFORE DELETE ON "targets" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."target_id" == "target_id") BEGIN SELECT RAISE(ABORT, 'genfkey9_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey9_update_referenced AFTER UPDATE OF target_id ON "targets" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."target_id" == "target_id") BEGIN SELECT RAISE(ABORT, 'genfkey9_update_referenced constraint failed'); END;
 
-- Triggers for foreign key mapping:
--
--     observations(session_id) REFERENCES sessions(session_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey10_insert_referencing BEFORE INSERT ON "observations" WHEN new."session_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "sessions" WHERE new."session_id" == "session_id") BEGIN SELECT RAISE(ABORT, 'genfkey10_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey10_update_referencing BEFORE UPDATE OF session_id ON "observations" WHEN new."session_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "sessions" WHERE new."session_id" == "session_id") BEGIN SELECT RAISE(ABORT, 'genfkey10_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey10_delete_referenced BEFORE DELETE ON "sessions" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."session_id" == "session_id") BEGIN SELECT RAISE(ABORT, 'genfkey10_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey10_update_referenced AFTER UPDATE OF session_id ON "sessions" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."session_id" == "session_id") BEGIN SELECT RAISE(ABORT, 'genfkey10_update_referenced constraint failed'); END;
 
-- Triggers for foreign key mapping:
--
--     observations(observer_id) REFERENCES observers(observer_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey11_insert_referencing BEFORE INSERT ON "observations" WHEN new."observer_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "observers" WHERE new."observer_id" == "observer_id") BEGIN SELECT RAISE(ABORT, 'genfkey11_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey11_update_referencing BEFORE UPDATE OF observer_id ON "observations" WHEN new."observer_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "observers" WHERE new."observer_id" == "observer_id") BEGIN SELECT RAISE(ABORT, 'genfkey11_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey11_delete_referenced BEFORE DELETE ON "observers" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."observer_id" == "observer_id") BEGIN SELECT RAISE(ABORT, 'genfkey11_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey11_update_referenced AFTER UPDATE OF observer_id ON "observers" WHEN EXISTS (SELECT 1 FROM "observations" WHERE old."observer_id" == "observer_id") BEGIN SELECT RAISE(ABORT, 'genfkey11_update_referenced constraint failed'); END;
 
-- Triggers for foreign key mapping:
--
--     sessions(observer_id) REFERENCES observers(observer_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey1_insert_referencing BEFORE INSERT ON "sessions" WHEN new."observer_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "observers" WHERE new."observer_id" == "observer_id") BEGIN SELECT RAISE(ABORT, 'genfkey1_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey1_update_referencing BEFORE UPDATE OF observer_id ON "sessions" WHEN new."observer_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "observers" WHERE new."observer_id" == "observer_id") BEGIN SELECT RAISE(ABORT, 'genfkey1_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey1_delete_referenced BEFORE DELETE ON "observers" WHEN EXISTS (SELECT 1 FROM "sessions" WHERE old."observer_id" == "observer_id") BEGIN SELECT RAISE(ABORT, 'genfkey1_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey1_update_referenced AFTER UPDATE OF observer_id ON "observers" WHEN EXISTS (SELECT 1 FROM "sessions" WHERE old."observer_id" == "observer_id") BEGIN SELECT RAISE(ABORT, 'genfkey1_update_referenced constraint failed'); END;
 
-- Triggers for foreign key mapping:
--
--     sessions(site_id) REFERENCES sites(site_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey2_insert_referencing BEFORE INSERT ON "sessions" WHEN new."site_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "sites" WHERE new."site_id" == "site_id") BEGIN SELECT RAISE(ABORT, 'genfkey2_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey2_update_referencing BEFORE UPDATE OF site_id ON "sessions" WHEN new."site_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "sites" WHERE new."site_id" == "site_id") BEGIN SELECT RAISE(ABORT, 'genfkey2_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey2_delete_referenced BEFORE DELETE ON "sites" WHEN EXISTS (SELECT 1 FROM "sessions" WHERE old."site_id" == "site_id") BEGIN SELECT RAISE(ABORT, 'genfkey2_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey2_update_referenced AFTER UPDATE OF site_id ON "sites" WHEN EXISTS (SELECT 1 FROM "sessions" WHERE old."site_id" == "site_id") BEGIN SELECT RAISE(ABORT, 'genfkey2_update_referenced constraint failed'); END;
 
-- Triggers for foreign key mapping:
--
--     targets(target_type_id) REFERENCES target_types(target_type_id)
--     on delete RESTRICT
--     on update RESTRICT
--
CREATE TRIGGER genfkey3_insert_referencing BEFORE INSERT ON "targets" WHEN new."target_type_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "target_types" WHERE new."target_type_id" == "target_type_id") BEGIN SELECT RAISE(ABORT, 'genfkey3_insert_referencing constraint failed'); END;
CREATE TRIGGER genfkey3_update_referencing BEFORE UPDATE OF target_type_id ON "targets" WHEN new."target_type_id" IS NOT NULL AND NOT EXISTS (SELECT 1 FROM "target_types" WHERE new."target_type_id" == "target_type_id") BEGIN SELECT RAISE(ABORT, 'genfkey3_update_referencing constraint failed'); END;
CREATE TRIGGER genfkey3_delete_referenced BEFORE DELETE ON "target_types" WHEN EXISTS (SELECT 1 FROM "targets" WHERE old."target_type_id" == "target_type_id") BEGIN SELECT RAISE(ABORT, 'genfkey3_delete_referenced constraint failed'); END;
CREATE TRIGGER genfkey3_update_referenced AFTER UPDATE OF target_type_id ON "target_types" WHEN EXISTS (SELECT 1 FROM "targets" WHERE old."target_type_id" == "target_type_id") BEGIN SELECT RAISE(ABORT, 'genfkey3_update_referenced constraint failed'); END;
