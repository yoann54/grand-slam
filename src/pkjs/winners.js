/*
 * winners.js — FALLBACK hors-ligne des vainqueurs de la dernière édition.
 *
 * La source VIVANTE est ../../winners.json, hébergé sur GitHub et récupéré au
 * lancement par index.js (cache localStorage). Ce module ne sert que si le
 * fetch distant échoue ET qu'aucun cache n'existe (1er lancement hors-ligne).
 * Garder ces valeurs ~à jour pour un fallback décent.
 *
 * Ordre des quadrants : 0 = AO, 1 = Roland-Garros, 2 = Wimbledon, 3 = US Open.
 * Noms de famille, sans accents (sécurité police Pebble).
 */
module.exports = {
  atp: ['Sinner', 'Alcaraz', 'Sinner', 'Alcaraz'],
  wta: ['Keys', 'Gauff', 'Swiatek', 'Sabalenka']
};
