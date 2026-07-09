/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 13 Jan 2023
*/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "Ble_central_coded.h"

LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/*
#define BT_UUID_LASER_VAL 
	BT_UUID_16_ENCODE(0xface)
*/
static const int8_t txp[9] = {8, 4, 0, -4, -8, -12, -16, -20, -40};  //for nrf52840

uint32_t var = 0;

static struct bt_conn *default_conn;
static struct bt_conn *default_conn2;

static uint16_t default_conn_handle;
uint8_t asset_tracking_array[MANUFACTURER_DATA_SIZE];

static void start_scan(void);

static const struct bt_le_adv_param *non_connectable_adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY,
			BT_GAP_ADV_VERY_SLOW_5, // 5s //
			BT_GAP_ADV_VERY_SLOW_5, // 5s //
			NULL);

static struct button_data_t {
	uint16_t  data;
	uint16_t len;
}*button_fifo_data;


struct ble_status {
		uint8_t request_pair:1;
		uint8_t is_connected:1;
		uint8_t request_disconnect:1;
		uint8_t is_advertising:1;
		uint8_t request_advertising:1;
		uint8_t request_sleep:1;
		uint8_t request_btn_data:1;
		uint8_t b7:1;
};

//struct ble_status status_flags;


struct ble_status status_flags = {
		.request_pair = 0,
		.is_connected = 0,
		.request_disconnect = 0,
		.is_advertising = 0,
		.request_advertising = 0,
		.request_sleep = 0,
		.request_btn_data = 0,
		.b7 = 0

};


void ScanLedBlink_expiry(struct k_timer *timer_id){
	gpio_pin_toggle_dt(&bleled);
}
void ScanLedBlink_stop(struct k_timer *timer_id){
	gpio_pin_set_dt(&bleled, 0);
}
K_TIMER_DEFINE(ScanLedBlink_timer, ScanLedBlink_expiry, ScanLedBlink_stop);

void PairBond_timer_expired(struct k_timer *timer_id){
	//printk("timer\n");
	status_flags.request_pair = 1;
	k_timer_stop(&ScanLedBlink_timer);
}
K_TIMER_DEFINE(PairBond_timer, PairBond_timer_expired, NULL);



//====================================================================


struct bond_checker{
	bt_addr_le_t addr;
	bool found;
};

struct manuf_data_t{
	uint8_t companyID1;
	uint8_t companyID2;
	uint8_t pti_seed;
	uint8_t mac_addr[6];
	uint8_t PT_Index0;
	uint8_t PT_Index1;
	union{
		struct{
			uint8_t b0:1;
			uint8_t b1:1;
			uint8_t b2:1;
			uint8_t b3:1;
			uint8_t b4:1;
			uint8_t b5:1;
			uint8_t b6:1;
			uint8_t b7:1;
		}bits;
		uint8_t byte;
	}status1;
	uint8_t adv_data[10];
};

static struct manuf_data_t manuf_data = {
	.companyID1 = SBD_ID1,
	.companyID2 = SBD_ID2,
	.pti_seed = SBD_PTI_SEED,
	.mac_addr[0] = 0x00,
	.mac_addr[1] = 0x01,
	.mac_addr[2] = 0x02,
	.mac_addr[3] = 0x03,
	.mac_addr[4] = 0x04,
	.mac_addr[5] = 0x05,
	.PT_Index0	= SBD_PT_Index0,
	.PT_Index1 = SBD_PT_Index1,
	.status1.byte = 0x00
};
static void set_manuf_data(struct bt_data *data){
	manuf_data.companyID1 = data->data[0];
	manuf_data.companyID2 = data->data[1];
	manuf_data.pti_seed = data->data[2];
	manuf_data.mac_addr[0] = data->data[3];
	manuf_data.mac_addr[1] = data->data[4];
	manuf_data.mac_addr[2] = data->data[5];
	manuf_data.mac_addr[3] = data->data[6];
	manuf_data.mac_addr[4] = data->data[7];
	manuf_data.mac_addr[5] = data->data[8];
	manuf_data.PT_Index0 = data->data[9];
	manuf_data.PT_Index1 = data->data[10];
	manuf_data.status1.byte = data->data[11];
}
static void set_mac_addr(char *macaddr)
{
	macaddr[3] = (uint8_t)NRF_FICR->DEVICEADDR[0];
	macaddr[4] = (uint8_t)(NRF_FICR->DEVICEADDR[0] >> 8);
	macaddr[5] = (uint8_t)(NRF_FICR->DEVICEADDR[0] >> 16);
	macaddr[6] = (uint8_t)(NRF_FICR->DEVICEADDR[0] >> 24);
	macaddr[7] = (uint8_t)NRF_FICR->DEVICEADDR[1];
	macaddr[8] =  (uint8_t)((NRF_FICR->DEVICEADDR[1] >> 8) | 0xC0); // 2MSB must be set 11
}


static const struct bt_data asset_tracking_sd[] = {
		BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, ASSET_TRACKING_DEVICE_NAME)
};
//static const struct bt_data asset_tracking_sd[] = {
//		BT_DATA_BYTES(BT_DATA_NAME_SHORTENED, ASSET_TRACKING_DEVICE_NAME)
//};
static struct manuf_data_t asset_tracking_data = {
	.companyID1 = SBD_ID1,
	.companyID2 = SBD_ID2,
	.pti_seed = SBD_PTI_SEED,
	.mac_addr[0] = 0x00,
	.mac_addr[1] = 0x01,
	.mac_addr[2] = 0x02,
	.mac_addr[3] = 0x03,
	.mac_addr[4] = 0x04,
	.mac_addr[5] = 0x05,
	.PT_Index0 = SBD_PT_Index0,
	.PT_Index1 = SBD_PT_Index1,
	.status1.byte = 0x5A
};

static void manuf_data_to_array(struct manuf_data_t *md, uint8_t *m_data_array, int size){
	m_data_array[0] = md->companyID1;
	m_data_array[1] = md->companyID2;
	m_data_array[2] = md->pti_seed;
	m_data_array[3] = md->mac_addr[0];
	m_data_array[4] = md->mac_addr[1];
	m_data_array[5] = md->mac_addr[2];
	m_data_array[6] = md->mac_addr[3];
	m_data_array[7] = md->mac_addr[4];
	m_data_array[8] = md->mac_addr[5];
	m_data_array[9] = md->PT_Index0;
	m_data_array[10] = md->PT_Index1;
	m_data_array[11] = md->status1.byte;
}



int asset_tracking_adv_start(void){
	int err;

	manuf_data_to_array(&asset_tracking_data, asset_tracking_array, MANUFACTURER_DATA_SIZE);
	set_mac_addr(asset_tracking_array);


	printk("\nMAC Adress is %x %x %x %x %x %x \n", asset_tracking_array[3],asset_tracking_array[4],asset_tracking_array[5],
												asset_tracking_array[6],asset_tracking_array[7],asset_tracking_array[8]);

	const struct bt_data asset_tracking_ad[] = {
		//BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		//	(CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
		//	(CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
		//BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR | BT_LE_AD_GENERAL),
		BT_DATA_BYTES(BT_DATA_UUID16_SOME,BT_UUID_16_ENCODE(BT_UUID_LASER_VAL)), // LASER Service 
		BT_DATA(BT_DATA_MANUFACTURER_DATA, asset_tracking_array, sizeof(asset_tracking_array))
		
	};
	err = bt_le_adv_start(non_connectable_adv_param, asset_tracking_ad, ARRAY_SIZE(asset_tracking_ad),
			      asset_tracking_sd, ARRAY_SIZE(asset_tracking_sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	return err;

}


// int asset_tracking_adv_update(void){
// 	int err;

// //	manuf_data_to_array(&asset_tracking_data, asset_tracking_array, MANUFACTURER_DATA_SIZE);
// 	set_mac_addr(asset_tracking_array);

// 	const struct bt_data asset_tracking_ad[] = {
// 		//BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
// 		//	(CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
// 		//	(CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
// 		//BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
// 		BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
// 		BT_DATA(BT_DATA_MANUFACTURER_DATA, asset_tracking_array, sizeof(asset_tracking_array)),
// 		BT_DATA_BYTES(BT_DATA_UUID16_SOME,
// 				BT_UUID_16_ENCODE(BT_UUID_LASER_VAL)), // LASER Service 
// 	};

// 	err = bt_le_adv_update_data(asset_tracking_ad, ARRAY_SIZE(asset_tracking_ad), asset_tracking_sd, ARRAY_SIZE(asset_tracking_sd));
// 	if (err) {
// 		printk("Advertising failed to update (err %d)\n", err);
// 		return;
// 	}
// 	return err;

// }

//====================================================================

static void set_tx_power(uint8_t handle_type, uint16_t handle, int8_t tx_pwr_lvl)
{
	struct bt_hci_cp_vs_write_tx_power_level *cp;
	struct bt_hci_rp_vs_write_tx_power_level *rp;
	struct net_buf *buf, *rsp = NULL;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
				sizeof(*cp));
	if (!buf) {
		printk("Unable to allocate command buffer\n");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = handle_type;
	cp->tx_power_level = tx_pwr_lvl;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
				   buf, &rsp);
	if (err) {
		uint8_t reason = rsp ?
			((struct bt_hci_rp_vs_write_tx_power_level *)
			  rsp->data)->status : 0;
		printk("Set Tx power err: %d reason 0x%02x\n", err, reason);
		return;
	}

	rp = (void *)rsp->data;
	printk("Actual Tx Power: %d\n", rp->selected_tx_power);

	net_buf_unref(rsp);
}
static void check_bonds(const struct bt_bond_info *info, void *user_data){
	struct bond_checker *bc = user_data;

	bt_addr_le_t *addr = &bc->addr;
	bt_addr_le_t *dest = &info->addr;
	// printk("Bond addr is %d, addr is %s",bc->found, bc->addr);
	if(!bt_addr_le_cmp(dest, addr)){
		printk("Bond Found\n");
		bc->found = true;
	}
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	//int i;
//printk("eir found\n");
	// printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	switch (data->type) {

		case BT_DATA_MANUFACTURER_DATA:
			
			if (data->data_len % sizeof(uint8_t) != 0U) {
				//printk("AD malformed MAD\n");
				return true;
			}else{
				struct bt_le_conn_param *param;
				struct bond_checker bc;
				int err;

				set_manuf_data(data);
				// printk("Data: 0x%02X,0x%02X,0x%02X,0x%02X\n", manuf_data.companyID1, manuf_data.companyID2, manuf_data.status1.byte, manuf_data.PT_Index1);
				if (manuf_data.companyID1 == SBD_ID1 && manuf_data.companyID2 == SBD_ID2  && manuf_data.PT_Index1 == SBD_PT_Index1 ){
					//printk("Data: 0x%02X\n", manuf_data.status1.byte);
					if ((manuf_data.status1.bits.b7 == 1) && (status_flags.request_pair == 0)){
						printk("Pair Request \n	");
						
						err = bt_le_scan_stop();
						if (err) {
							//printk("Stop LE scan failed (err %d)\n", err);
							break;
						}

						err = bt_unpair(BT_ID_DEFAULT, NULL);
						//err = bt_unpair(BT_ID_DEFAULT, addr);
						if (err) {
							//LOG_ERR("Unable to remove bond information: %d", err);
							//printk("Unable to remove bond information:\n");
						}

						param = BT_LE_CONN_PARAM_DEFAULT;
						err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
						if (err) {
							//printk("Create conn failed (err %d)\n", err);
							start_scan();
						}
						printk(" Unit bonded new pair request \n");

						return false;
					}else{
						
						bc.addr = (*addr);
						bc.found = false;
						bt_foreach_bond(BT_ID_DEFAULT, check_bonds, &bc);

						if(bc.found){
							printk("Auto-connect\n");
							err = bt_le_scan_stop();

							if (err) {
								//printk("Stop LE scan failed (err %d)\n", err);
								break;
							}
							param = BT_LE_CONN_PARAM_DEFAULT;
							err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
										param, &default_conn);
							if (err) {
								//printk("Create conn failed (err %d)\n", err);
								start_scan();
							}
							printk("Unit bonded on autopair\n");

							return false;
						}else{
							// printk("Unit not bonded from auto pair\n");
						}
					}
					//k_sleep(K_MSEC(1000));
				}
			}
			// Parse Data
	}
	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	// printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	//        dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_GAP_ADV_TYPE_ADV_IND ||
	    type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
	//printk("device found\n");
}

static void start_scan(void)
{
	int err;

	if(status_flags.request_sleep ==0)
	{


		/* Use active scanning and disable duplicate filtering to handle any
		* devices that might update their advertising data at runtime. */
		struct bt_le_scan_param scan_param = {
			.type       = BT_LE_SCAN_TYPE_ACTIVE,
			.options    = BT_LE_SCAN_OPT_NONE,
			.interval   = BT_GAP_SCAN_FAST_INTERVAL,
			.window     = BT_GAP_SCAN_FAST_WINDOW,
		};

		err = bt_le_scan_start(&scan_param, device_found);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			return;
		}

		// printk("Scanning successfully started\n");
		
	}


}

struct bt_nus_client nus_client;
//after BLE sent, free buf, k_sem_give

static void ble_data_sent(struct bt_nus_client *nus, uint8_t err,
					const uint8_t *const data, uint16_t len)
{
// 	ARG_UNUSED(nus);

// 	struct uart_data_t *buf;

// 	/* Retrieve buffer context. */
// 	buf = CONTAINER_OF(data, struct uart_data_t, data);
// 	k_free(buf);

//	k_sem_give(&nus_write_sem);

	if (err) {
		// LOG_WRN("ATT error code: 0x%02X", err);
	}
}
static uint8_t ble_data_received(struct bt_nus_client *nus,
						const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(nus);

	// int err;

	// for (uint16_t pos = 0; pos != len;) {
	// 	struct uart_data_t *tx = k_malloc(sizeof(*tx));

	// 	if (!tx) {
	// 		// LOG_WRN("Not able to allocate UART send data buffer");
	// 		return BT_GATT_ITER_CONTINUE;
	// 	}

	// 	/* Keep the last byte of TX buffer for potential LF char. */
	// 	size_t tx_data_size = sizeof(tx->data) - 1;

	// 	if ((len - pos) > tx_data_size) {
	// 		tx->len = tx_data_size;
	// 	} else {
	// 		tx->len = (len - pos);
	// 	}

	// 	memcpy(tx->data, &data[pos], tx->len);

	// 	pos += tx->len;

	// 	/* Append the LF character when the CR character triggered
	// 	 * transmission from the peer.
	// 	 */
	// 	if ((pos == len) && (data[len - 1] == '\r')) {
	// 		tx->data[tx->len] = '\n';
	// 		tx->len++;
	// 	}
	uint8_t temp_buf[3]; 
	memcpy(temp_buf, data, 3);
	/****************************Test remove later *********************************/
	for (int i =0 ;i < 3; i++)
	{
		printk("data is %x ", temp_buf[i]);					//@KJ
	}
	printk (" %u \n", var);
	var ++;
	/***************************************************************************/

	button_fifo_data = k_malloc(sizeof(struct button_data_t));
	uint16_t keypad_data = 0x0000;
	keypad_data = temp_buf[2];
	keypad_data |= temp_buf[1] << 8;
	// if(temp_buf[1] == 0 || temp_buf[2] == 0)
	// {
	// 	k_free(button_fifo_data);
	// 	return;
	// }	
	button_fifo_data->data = keypad_data;
	button_fifo_data->len = 2;
	if(keypad_data & 0x8000)
	{
		printk("Received Motor Stop command \n");
		motor_stop();
		return 1;
	}
	else
	{	
		k_fifo_put(&button_fifo, button_fifo_data);
	}

	// k_free(button_fifo_data);

	// }



	return BT_GATT_ITER_CONTINUE;
}


static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	struct bt_nus_client *nus = context;
	//LOG_INF("Service discovery completed");
	printk("Service discovery completed.\n");

	bt_gatt_dm_data_print(dm);

	bt_nus_handles_assign(dm, nus);
	bt_nus_subscribe_receive(nus);

	bt_gatt_dm_data_release(dm);
	k_timer_stop(&ScanLedBlink_timer);
	gpio_pin_set_dt(&bleled, 1);
	status_flags.is_connected = 1;
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	// LOG_INF("Service not found");
	//printk("Service not found.\n");

}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	// LOG_WRN("Error while discovering GATT database: (%d)", err);
	//printk("Error while discovering GATT database:.\n");

}

static struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != default_conn) {
		return;
	}

	err = bt_gatt_dm_start(conn,
			       BT_UUID_NUS_SERVICE,
			       &discovery_cb,
			       &nus_client);
	if (err) {
		// LOG_ERR("could not start the discovery procedure, error "
			// "code: %d", err);
	}
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		// LOG_INF("MTU exchange done");
	} else {
		// LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		//LOG_INF("Failed to connect to %s (%d)", log_strdup(addr),
		//	conn_err);
		printk("Failed to connect to.\n");
		

		if (default_conn == conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;

		//	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			start_scan();
			printk("scan Started from Connected\n");

			//printk("Scanning start.\n");
	//		if (err) {
	//			LOG_ERR("Scanning failed to start (err %d)",
	//				err);
	//			printk("Scanning failed to start (err.\n");
	//		}
		}

		return;
	}

//default_conn = bt_conn_ref(conn);
		err = bt_hci_get_conn_handle(default_conn,
					     &default_conn_handle);
		if (err) {
			printk("No connection handle (err %d)\n", err);
		} else {
			/* Send first at the default selected power */
			bt_addr_le_to_str(bt_conn_get_dst(conn),
							  addr, sizeof(addr));
		//	printk("Connected via connection (%d) at %s\n",
		//	       default_conn_handle, addr);
		//	get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
		//		     default_conn_handle, &txp);
		//	printk("Connection (%d) - Initial Tx Power = %d\n",
		//	       default_conn_handle, txp);

		//	set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
		//		     default_conn_handle,
		//		     BT_HCI_VS_LL_TX_POWER_LEVEL_NO_PREF);

			set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
				     default_conn_handle,
				     txp[0]);
		//	get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
		//		     default_conn_handle, &txp);
		//	printk("Connection (%d) - Tx Power = %d\n",
		//	       default_conn_handle, txp);
		}

	//LOG_INF("Connected: %s", log_strdup(addr));
	printk("connected.\n");
	static struct bt_gatt_exchange_params exchange_params;

	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		//LOG_WRN("MTU exchange failed (err %d)", err);
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	//printk("bt_conn_set_security\n");
	if (err) {
	//	LOG_WRN("Failed to set security: %d", err);
		//printk("Failed to set security\n");

	//	gatt_discover(conn);
	}

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
	//	LOG_ERR("Stop LE scan failed (err %d)", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
//	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	//LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr),
	//	reason);

	printk("disconnected.\n");		//@KJ
	motor_stop();
	status_flags.is_connected = 0;
//	if (default_conn != conn) {
//		return;
//	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
	gpio_pin_set_dt(&bleled, 0);

	//err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	start_scan();
	printk("Scan started from disconnected\n");
	

	//printk("Scanning start from disconnect.\n");
//	if (err) {
//		LOG_ERR("Scanning failed to start (err %d)",
//			err);
//		printk("Scanning failed to start.\n");
//	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
	//	LOG_INF("Security changed: %s level %u", log_strdup(addr),
	//		level);

		printk("Security changed.\n");

	} else {
		
	//	LOG_WRN("Security failed: %s level %u err %d", log_strdup(addr),
	//		level, err);

	//	printk("Security failed.\n");



		bt_conn_disconnect	(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);		








		//err = bt_unpair(BT_ID_DEFAULT, conn);
		//err = bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
		//err = bt_unpair(BT_ID_DEFAULT, addr);


	//	err = bt_unpair(BT_ID_DEFAULT, NULL);
	//	if (err) {
	//		LOG_ERR("Unable to remove bond information: %d", err);
	//		printk("Unable to remove bond information:\n");
	//	}
	
		if (default_conn == conn) {
			//bt_conn_unref(default_conn);
			//default_conn = NULL;

			//err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			start_scan();
			printk("Scan started from security Failed \n");

		//	printk("Scanning start in security fail.\n");
		//	if (err) {
		//		LOG_ERR("Scanning failed to start (err %d)",
		//			err);
		//		printk("Scanning failed to start (err.\n");
		//	}
		}
		else{
			//default_conn = NULL;

			//err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			start_scan();
			printk("Scan started from security Failed 2\n");

		//	printk("Scanning start in security fail.\n");
			if (err) {
		//		LOG_ERR("Scanning failed to start (err %d)",
		//			err);
		//		printk("Scanning failed to start (err.\n");
			}

		}
	
		
		return;
		
	}
	gatt_discover(conn);
}
/*
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};
*/
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};







static int nus_client_init(void)
{
	int err;
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
			.sent = ble_data_sent,
		}
	};

	err = bt_nus_client_init(&nus_client, &init);
	if (err) {
	//	LOG_ERR("NUS Client initialization failed (err %d)", err);
		return err;
	}

	// LOG_INF("NUS Client module initialized");
	// printk("NUS Client module initialized");
	return err;
}
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	//LOG_INF("Pairing cancelled: %s", log_strdup(addr));
//	printk("Pairing cancelled:\n");
}


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	//LOG_INF("Pairing completed: %s, bonded: %d", log_strdup(addr),
	//	bonded);

//	printk("Pairing completed, bonded:\n");
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	//LOG_WRN("Pairing failed conn: %s, reason %d", log_strdup(addr),
	//	reason);

//	printk("Pairing failed:\n");
	//bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

void Bluetooth_initialize(void)
{
	int err;
	status_flags.request_pair = 0;
	// status_flags = 0;
	// k_timer_start(&PairBond_timer, K_SECONDS(60), K_NO_WAIT);
	// k_timer_start(&ScanLedBlink_timer, K_MSEC(500), K_MSEC(500));
	// printk(" here 01\n");
	
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		//LOG_ERR("Failed to register authorization callbacks.");
		return;
	}
// printk(" here 02\n");

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		//printk("Failed to register authorization info callbacks.\n");
		return;
	}
	bt_conn_cb_register(&conn_callbacks);	
	// printk(" here 03\n");

	err = bt_enable(NULL);
	if (err) {
		//LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	//LOG_INF("Bluetooth initialized");
	printk("Bluetooth initialized:\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = nus_client_init();
	if (err) {
		//LOG_ERR("Failed to initialize ble uart.");
		return;
	}
	
	asset_tracking_adv_start();
	//printk("Starting Bluetooth Central UART example\n");
	
	//err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	start_scan();
	
	// printk("Scan started from BLE INIT() \n");
	if (err) {
		// LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}
	// status_flags.request_pair = 1;

	//LOG_INF("Scanning successfully started");
	// printk("Scanning successfully started\n");


	//printk("Get Tx power level ->");
	//get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_SCAN, 0, &txp_get);
	//printk("-> default TXP = %d\n", txp_get);

	//printk("Set Tx power level to %d\n", txp[0]);
	set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_SCAN, 0, txp[0]);
	set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, 0, txp[0]);  

}

void BleComms_start(void)
{
	
	
	if(status_flags.is_connected == 1)
	{
		gpio_pin_set_dt(&bleled, 1);
	}
	else
	{
		status_flags.request_sleep = 0;
		status_flags.request_pair = 0;
		k_timer_start(&PairBond_timer, K_SECONDS(60), K_NO_WAIT);
		k_timer_start(&ScanLedBlink_timer, K_MSEC(500), K_MSEC(500));
		start_scan();
		set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_SCAN, 0, txp[0]);
	}
	
}

void Stop_BleComm(void)
{	
	int err;
	status_flags.request_sleep = 1;
	k_timer_stop(&PairBond_timer);
	k_timer_stop(&ScanLedBlink_timer);
	
	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}	

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Disconnect failed (err %d)", err);
	}	
	set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_SCAN, 0, txp[8]);
	
}

