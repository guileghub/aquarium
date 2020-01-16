import { Injectable } from '@angular/core';
import { Subject } from 'rxjs';
import { webSocket, WebSocketSubject } from "rxjs/webSocket";

@Injectable({
	providedIn: 'root'
})

export class ControllerService {
	connected: boolean = false;
	subject: WebSocketSubject<any>;
	temperatures = new Subject<number>();
	temperatures$ = this.temperatures.asObservable();
	constructor() { }
	connect(url: string) {
		this.subject = webSocket({
			url: url,
			openObserver: {
				next: () => {
					console.log('connetion ok');
					this.connected = true;
				}
			},
			closeObserver: {
				next: () => {
					console.log('connetion closed');
					this.connected = false;
				}
			},
		});
		this.subject.subscribe((message: any) => {
			this.Parse(message);
		},
			err => console.log(err),
			() => console.log('complete')
		);
	}
	Parse(message: any) {
		//console.warn(message);
		if (typeof (message.temperature) == 'number') {
			this.temperatures.next(message.temperature);
			return;
		}
		console.error(message);
	}
}
