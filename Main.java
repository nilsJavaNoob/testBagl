public class Main {

	 public static void main(String[] args){
			System.out.println("Hu");
			Owner owner = new Owner("Vasya");
			House house = new House(16,4);
			Appartment a = new Appartment(2);
			
			house.settle(owner);
			
			System.out.println("=============");
			System.out.println(house.toString());
			System.out.println(a.toString());
			//System.out.println(owner.toString());
			System.out.println("=============");
	}
}//class

//git remote add tbagl https://github.com/nilsJavaNoob/testBagl.git
//git push tbagl