public class Main {

	 public static void main(String[] args){
			System.out.println("Hi");
			Owner owner = new Owner("Vasya");
			House house = new House(16,4);
			house.settle(owner);
			
			System.out.println("=============");
			System.out.println(house.toString());
			//System.out.println(owner.toString());
			System.out.println("=============");
	}
}//class

//git remote add tbagl https://github.com/nilsJavaNoob/testBagl.git
//git push tbagl