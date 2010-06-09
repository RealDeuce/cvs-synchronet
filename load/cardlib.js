// $Id$
/*
	A library for dealing with a traditional deck of cards in various ways.
	Licensed under the GPLv2 by Randolph E. Sommerfeld <cyan@rrx.ca>
*/

const CLUBS = 0;
const DIAMONDS = 1;
const HEARTS = 2;
const SPADES = 3;

const SUITS = new Array(CLUBS, DIAMONDS, HEARTS, SPADES);
const SUIT_CHAR = new Array("c", "d", "h", "s");
const SUIT_STR = new Array("clubs", "diamonds", "hearts", "spades");

const TWO = 0;
const THREE = 1;
const FOUR = 2;
const FIVE = 3;
const SIX = 4;
const SEVEN = 5;
const EIGHT = 6;
const NINE = 7;
const TEN = 8;
const JACK = 9;
const QUEEN = 10;
const KING = 11;
const ACE = 12;

const VALUES = new Array(TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN,
	JACK, QUEEN, KING, ACE);
const VALUE_CHAR = new Array("2", "3", "4", "5", "6", "7", "8", "9", "T", "J",
	"Q", "K", "A");
const VALUE_STR = new Array("deuce", "trey", "four", "five", "six", "seven",
	"eight", "nine", "ten", "jack", "queen", "king", "ace");

const HIGH_CARD = (1<<0);
const PAIR = (1<<1);
const TWO_PAIR = (1<<2);
const THREE_OF_A_KIND = (1<<3);
const STRAIGHT = (1<<4);
const FLUSH = (1<<5);
const FULL_HOUSE = (1<<6);
const FOUR_OF_A_KIND = (1<<7);
const STRAIGHT_FLUSH = (1<<8);
const ROYAL_FLUSH = (1<<9);

const HC = 0;
const PR = 1;
const TP = 2;
const TK = 3;
const ST = 4;
const FL = 5;
const FH = 6;
const FK = 7;
const SF = 8;
const RF = 9;

const RANKS = new Array("high card", "a pair", "two pair", "three of a kind",
	"a straight", "a flush", "a full house", "four of a kind", "a straight flush",
	"a royal flush");

function Card(value, suit) {
	this.value = value;
	this.suit = suit;
	this.color = "";
	this.char = VALUE_CHAR[value] + SUIT_CHAR[suit];
	
	if(suit==HEARTS || suit==DIAMONDS)	this.color=String.fromCharCode(3)+"5";
	else if(suit==SPADES || suit==CLUBS)	this.color=String.fromCharCode(3)+"1";
}

function Deck() {
	/* Create a new deck */
	this.cards = new Array();
	for (s in SUITS) {
		for (v in VALUES) {
			var card = new Card(v, s);
			this.cards.push(card);
		}
	}
	this.ptr = 0;	/* Location we're at in the deck */
	this.shuffle = Deck_Shuffle;
	this.deal = Deck_Deal;
}

function Deck_Shuffle() {
	var unshuffled = this.cards;	/* Copy the deck as it is now */
	this.cards = new Array();		/* Start the deck afresh */
	while (unshuffled.length > 0) {
		var rnd = Math.floor(Math.random() * unshuffled.length);
		this.cards.push(unshuffled[rnd]);
		unshuffled.splice(rnd, 1);
	}
	this.ptr = 0;
}

function Deck_Deal() {
	var c = this.cards[this.ptr];
	this.ptr++;
	return c;
}

function Hunt_Straight(cards,depth) {
	var got_straight = false;
	for (c in cards) {
		var compare = parseInt(cards[c].value) + 1;
		if (cards[depth].value == compare) {
			if (depth == 4)
				return true;
			got_straight = Hunt_Straight(cards,depth+1);
		}
		if (got_straight)
			return true;
	}
}

/* This function accepts any number of cards in as a true array */
function Rank(cards) {
	var hand=new Object();
	hand.group = new Object();
	hand.group[0] = 0; /* Rank - no value */
	hand.rank = hand.group[0];
	hand.str = "";

	/* First, count up the cards. */
	var count = new Object();
	for (c in cards) {
		if (!count[cards[c].value])
			count[cards[c].value] = 0;
		count[cards[c].value]++;
	}
	
	if (count[ACE] && count[TWO] && count[THREE] && count[FOUR]
		&& count[FIVE]) {
			hand.rank |= STRAIGHT;
			hand.str = "the wheel";
	}
	/* Only hunt straights 2 through 10 */
	for (s=2; s<10; s++) {
		if (count[s] && count[s+1] && count[s+2] && count[s+3] && count[s+4]) {
			hand.rank |= STRAIGHT;
			hand.str = "a straight to the " + VALUE_STR[s+3];
		}
	}

	/* Recount the cards, but sort by suit */
	var suited_count = new Object();
	for (s in SUITS) {
		suited_count[SUITS[s]] = new Object();
	}
	for (c in cards) {
		if (!suited_count[cards[c].suit][cards[c].value])
			suited_count[cards[c].suit][cards[c].value] = 0;
		suited_count[cards[c].suit][cards[c].value]++;
	}
	for (u in suited_count) {
		if (true_array_len(suited_count[u]) >= 5) {
			hand.rank |= FLUSH;
			if (hand.rank&STRAIGHT) { /* Possible straight flush on board */
				if(suited_count[u][ACE] && suited_count[u][TWO]
					&& suited_count[u][THREE] && suited_count[u][FOUR]
					&& suited_count[u][FIVE]) {
						hand.rank |= STRAIGHT_FLUSH;
						hand.str = "the wheel";
				}
				/* Only hunt straights 2 through 10 */
				for (s=TWO; s<TEN; s++) {
					if (suited_count[u][s] && suited_count[u][s+1]
						&& suited_count[u][s+2] && suited_count[u][s+3]
						&& suited_count[u][s+4]) {
							hand.rank |= STRAIGHT_FLUSH;
							hand_str = "a straight to the " + VALUE_STR[s+3];
							if (s == TEN) {
								hand.rank |= ROYAL_FLUSH;
							}
					}
				}
			}
		}
	}

	/* Not Used, High Card, Pair, Three of a Kind, Four of a Kind */
	var pair_types = new Array(0, 1, 2, 3, 4);
	for (p in pair_types) {
		pair_types[p] = new Array();
	}
	for (c in count) {
		if (pair_types[p][0] && (pair_types[p][0] < c) )
			pair_types[count[c]].push(c);
		else
			pair_types[count[c]].unshift(c);
	}
	if (true_array_len(pair_types[4]) > 0)
		hand.rank |= FOUR_OF_A_KIND;
	else if (   (true_array_len(pair_types[3]) > 0)
			 && (true_array_len(pair_types[2]) > 0) ) {
		hand.rank |= FULL_HOUSE;
	} else if (true_array_len(pair_types[3]) > 0)
		hand.rank |= THREE_OF_A_KIND;
	else if (true_array_len(pair_types[2]) > 1)
		hand.rank |= TWO_PAIR;
	else if (true_array_len(pair_types[2]) == 1)
		hand.rank |= PAIR;

	if (hand.rank&ROYAL_FLUSH)
		hand.rank=RF;
	else if (hand.rank&STRAIGHT_FLUSH)
		hand.rank=SF;
	else if (hand.rank&FOUR_OF_A_KIND) 
		hand.rank=FK;
	else if (hand.rank&FULL_HOUSE)
		hand.rank=FH;
	else if (hand.rank&FLUSH)
		hand.rank=FL;
	else if (hand.rank&STRAIGHT)
		hand.rank=ST;
	else if (hand.rank&THREE_OF_A_KIND)
		hand.rank=TK;
	else if (hand.rank&TWO_PAIR)
		hand.rank=TP;
	else if (hand.rank&PAIR)
		hand.rank=PR;
	else
		hand.rank=HC;

	if(hand.rank!=ST)
		hand.str=RANKS[hand.rank];

	/* Consolidate "count" array and reverse. 
	   Only consolidate cards where count=1 */
	var consolidated=new Array;
	for(c in count) {
		if (count[c]==1)
			consolidated.push(c);
	}
	hand.high=consolidated.reverse();	
	consolidated = consolidated.sort(sortNumber);

	switch(hand.rank) {
		case HC:
		case FL: /* Yes, FL really gets treated like HC. */
			hand.group[1] = consolidated.pop();
			hand.group[2] = consolidated.pop();
			hand.group[3] = consolidated.pop();
			hand.group[4] = consolidated.pop();
			hand.group[5] = consolidated.pop();
			break;
		case PR:
			hand.group[1] = hunt_count(count, 2);
			hand.group[2] = consolidated.pop();
			hand.group[3] = consolidated.pop();
			hand.group[4] = consolidated.pop();
			break;
		case TP:
			var first_pair = hunt_count(count, 2);
			delete count[first_pair];
			var second_pair = hunt_count(count, 2);
			if (first_pair > second_pair) {
				hand.group[1] = first_pair;
				hand.group[2] = second_pair;
			} else {
				hand.group[1] = second_pair;
				hand.group[2] = first_pair;
			}
			hand.group[3] = consolidated.pop();
			break;
		case TK:
			hand.group[1] = hunt_count(count, 3);
			hand.group[2] = consolidated.pop();
			break;
		case ST:
		case SF:
			hand.group[1] = consolidated.pop();
			break;
		case FH:
			hand.group[1] = hunt_count(count, 3);
			hand.group[2] = hunt_count(count, 2);
			hand.group[3] = consolidated.pop();
			break;
		case FK:
			hand.group[1] = hunt_count(count, 4);
			hand.group[2] = consolidated.pop();
			break;
		case RF:
		default:
			break;
	}

	return hand;
}

function hunt_count(cards, count) {
	for (c in cards) {
		if (cards[c] == count)
			return c;
	}
}

function sortNumber(a,b) {
	return a - b;
}

function Sort(cards) {
	/* Sort the cards from top (highest) to bottom (lowest) */
	var sorted = new Array();
	sorted.push(cards[0]);
	for (c=1; c<cards.length; c++) {
		var c_bogus_rank = parseInt(cards[c].value + cards[c].suit);
		for (s in sorted) {
			var s_bogus_rank = parseInt(sorted[s].value + sorted[s].suit);
			if (c_bogus_rank > s_bogus_rank) {
				sorted.splice(s,0,cards[c]);
				break;
			}
			if (parseInt(s)+1 == sorted.length)
				sorted.push(cards[c]);
		}
	}
	return sorted;
}

/* Stolen from the Synchronet IRCd. */
function true_array_len(my_array) {
    var counter = 0;
    for (i in my_array) {
        counter++;
    }
    return counter;
}

