server: auctionserver.o
	gcc -o server -g auctionserver.o

seller: seller.o
	gcc -o seller -g seller.o

bidder: bidder.o
	gcc -o bidder -g bidder.o

auctionserver.o: auctionserver.c
	gcc -g -c  auctionserver.c

bidder.o: bidder.c
	gcc -g -c  bidder.c

seller.o: seller.c
	gcc -g -c seller.c

clean:
	rm -f *.o server bidder seller
