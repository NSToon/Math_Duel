package Slave;

import java.util.*;

public class Game2 {
    public static void main(String[] args) {
        Deck deck = new Deck();
        System.out.println(deck.getCards());
        deck.shuffle();
        System.out.println(deck.getCards());
        
        // List<Card>[] hands = deck.deal(4);
        // for (int i = 0; i < hands.length; i++) {
        //     Collections.sort(hands[i]);
        //     System.out.println("Player " + (i+1) + ": " + hands[i]);
        // }

        System.out.println("=== เกม Slave (President) ===");
        System.out.println("กติกา: ทิ้งไพ่ให้หมดก่อนเป็นผู้ชนะ");
        List<Card>[] hands = deck.deal(4);  // แจกไพ่ 4 มือ
        Player[] players = new Player[4];

        for (int i = 0; i < 4; i++) {
            players[i] = new Player("Player " + (i + 1));
            for (int j = 0; j < hands[i].size(); j++) {
                Card c = hands[i].get(j);
                players[i].addCard(c);
            }
            players[i].sortHand();
        }

// แสดงไพ่หลังแจกและเรียง
        for (Player p : players) {
            System.out.println(p.getName() + ":");
            p.showHand();
            System.out.println();
        }
    }
}

class Card implements Comparable<Card> {
    public String type;
    public int rank;    

    public Card(String type, int rank) {
        this.type = type;
        this.rank = rank;
    }
    
    @Override
    public int compareTo(Card other) {
        return Integer.compare(this.rank, other.rank);
    }

    @Override
    public String toString() {
        String name;
        if (rank == 11){
            name = "J";
        }else if (rank == 12){
            name = "Q";
        }else if (rank == 13){
            name = "K";
        }else if (rank == 14){
            name = "A";
        }else if (rank == 15){
            name = "2";
        }else {
            name = String.valueOf(rank);
        }

        return name + type;
    }

}

class Deck {
    private List<Card> cards;
    int index = 0;

    public Deck() {
        cards = new ArrayList<>();
        String[] types = {"C", "D", "H", "S"}; // ดอกไพ่ (Suits)
        for (String type : types) {
            for (int rank = 3; rank <= 15; rank++) {
                cards.add(new Card(type, rank));
            }
        }
    }
    public List<Card> getCards() {
        return cards;
    }
     public void shuffle(){
        Collections.shuffle(cards);
    }
    public List<Card>[] deal(int players){
        List<Card>[] hands = new ArrayList[players];
        for (int i = 0; i < players; i++){
            hands[i] = new ArrayList<>();
        }
        for (int round = 0; round < cards.size()/players; round++) {
            for (int player = 0; player < players; player++) {
                hands[player].add(cards.get(index));
                index++;
            }
        }
        return hands;

    }
}

class Player {
    private String name;            
    private List<Card> hand;
    private boolean passed;
    private boolean finished;

    public Player(String name) {
        this.name = name;
        this.hand = new ArrayList<>();
        this.passed = false;
        this.finished = false;
    }

    public String getName() {
        return name;
    }
    public List<Card> getHand() {
        return hand;
    }
    public void addCard(Card c) {
        hand.add(c); // เพิ่มไพ่เข้ามือ
    }
    public void sortHand() {
        Collections.sort(hand); // เรียงไพ่ (ใช้ compareTo ของ Card)
    }
    public void removeCards(List<Card> played) {
        hand.removeAll(played); // เอาไพ่ที่ลงออกจากมือ
        if (hand.isEmpty()) {
            finished = true; // ถ้าไม่มีไพ่แล้ว แสดงว่าชนะไปแล้ว
        }
    }

    public void setPassed(boolean p) {
        this.passed = p; // เซ็ตสถานะ "ผ่าน"
    }

    public boolean hasPassed() {
        return passed;
    }

    public boolean isFinished() {
        return finished;
    }

    public void showHand() {
        // แสดงไพ่ในมือแบบดูง่าย (พร้อมเลข index สำหรับเลือก)
        for (int i = 0; i < hand.size(); i++) {
            System.out.print("(" + i + ")" + hand.get(i) + " ");
        }
        System.out.println();
    }
    @Override
    public String toString() {
        return name + "'s hand: " + hand;
    }

}
